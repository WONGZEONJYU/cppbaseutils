#ifndef X_OBJECT_P_P_HPP
#define X_OBJECT_P_P_HPP 1

#include <XObject/xobject_p.hpp>
#include <XHelper/xhelper.hpp>
#include <list>
#include <unordered_map>
#include <iostream>
#include <type_traits>
#include <XGlobal/xtypeinfo.hpp>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

struct XObjectPrivate::ConnectionList {
    XAtomicPointer<Connection> first{},last{};
};
static_assert(std::is_trivially_destructible_v<XObjectPrivate::ConnectionList>);
X_DECLARE_TYPEINFO(XObjectPrivate::ConnectionList, X_RELOCATABLE_TYPE);

struct XObjectPrivate::TaggedSignalVector {
    xuintptr c;

    TaggedSignalVector() = default;

    explicit TaggedSignalVector(std::nullptr_t) noexcept
    : c(0) {}
    explicit TaggedSignalVector(Connection * const v) noexcept
    : c(reinterpret_cast<xuintptr>(v))
    { X_ASSERT(v && (reinterpret_cast<xuintptr>(v) & 0x1) == 0);   }

    explicit TaggedSignalVector(SignalVector * const v) noexcept
    : c(reinterpret_cast<xuintptr>(v) | static_cast<xuintptr>(1u))
    { X_ASSERT(v); }

    explicit operator SignalVector *() const noexcept {
        if (c & 0x1) {
            return reinterpret_cast<SignalVector *>(c & ~static_cast<xuintptr>(1u));
        }
        return nullptr;
    }
    explicit operator Connection *() const noexcept {
        return reinterpret_cast<Connection *>(c);
    }
    operator uintptr_t() const noexcept { return c; }
};

struct XObjectPrivate::ConnectionOrSignalVector {
    union {
        // linked list of orphaned connections that need cleaning up
        TaggedSignalVector nextInOrphanList;
        // linked list of connections connected to slots in this object
        Connection *next;
    };
};
static_assert(std::is_trivial_v<XObjectPrivate::ConnectionOrSignalVector>);

struct XObjectPrivate::Connection : ConnectionOrSignalVector {
    // linked list of connections connected to slots in this object, next is in base class
    Connection **prev{};
    // linked list of connections connected to signals in this object
    XAtomicPointer<Connection> nextConnectionList{};
    Connection *prevConnectionList{};

    XObject *sender{};
    XAtomicPointer<XObject> receiver{};

    XPrivate::XSignalSlotBase *slotObj{};

    XAtomicPointer<const int> argumentTypes{};
    XAtomicInt ref_{
            2
    }; // ref_ is 2 for the use in the internal lists, and for the use in QMetaObject::Connection
    uint id {};
    ushort method_offset{};
    ushort method_relative{};
    signed int signal_index : 27; // In signal range (see QObjectPrivate::signalIndex())
    ushort connectionType : 2; // 0 == auto, 1 == direct, 2 == queued, 3 == blocking
    ushort isSlotObject : 1;
    ushort ownArgumentTypes : 1;
    ushort isSingleShot : 1;
    Connection()
    : ConnectionOrSignalVector(),signal_index{},connectionType{},isSlotObject{},
    ownArgumentTypes(true),isSingleShot{} { }

    ~Connection();

    [[maybe_unused]] int method() const {
        X_ASSERT(!isSlotObject);
        return method_offset + method_relative;
    }
    void ref() { ref_.ref(); }
    void freeSlotObject() {
        if (isSlotObject) {
            slotObj->destroyIfLastRef();
            isSlotObject = false;
        }
    }
    void deref() {
        if (!ref_.deref()) {
            X_ASSERT(!receiver.loadRelaxed());
            X_ASSERT(!isSlotObject);
            delete this;
        }
    }
};
X_DECLARE_TYPEINFO(XObjectPrivate::Connection, X_RELOCATABLE_TYPE);

struct XObjectPrivate::SignalVector : ConnectionOrSignalVector {
    xuintptr allocated;
    // ConnectionList signals[]
    [[nodiscard]] ConnectionList &at(int const i) { return reinterpret_cast<ConnectionList *>(this + 1)[i + 1]; }
    [[nodiscard]] const ConnectionList &at(int const i) const {
        return reinterpret_cast<const ConnectionList *>(this + 1)[i + 1];
    }
    [[nodiscard]] int count() const { return static_cast<int>(allocated); }
};
static_assert(std::is_trivial_v<XObjectPrivate::SignalVector>); // it doesn't need to be, but it helps

struct XObjectPrivate::ConnectionData {
    // the id below is used to avoid activating new connections. When the object gets
    // deleted it's set to 0, so that signal emission stops
    XAtomicInteger<uint> currentConnectionId{};
    XAtomicInt ref{};
    XAtomicPointer<SignalVector> signalVector{};
    Connection *senders {};
    Sender *currentSender {}; // object currently activating the object
    std::atomic<TaggedSignalVector> orphaned {};

    ~ConnectionData() {
        X_ASSERT(ref.loadRelaxed() == 0);
        if (const auto c{orphaned.exchange(TaggedSignalVector(nullptr), std::memory_order_relaxed)}){
            deleteOrphaned(c);
        }
        if (const auto v{signalVector.loadRelaxed()}) {
            v->~SignalVector();
            free(v);
        }
    }

    // must be called on th320
    // e senders connection data
    // assumes the senders and receivers lock are held
    void removeConnection(Connection *);
    enum LockPolicy {
        NeedToLock,
        // Beware that we need to temporarily release the lock
        // and thus calling code must carefully consider whether
        // invariants still hold.
        AlreadyLockedAndTemporarilyReleasingLock
    };
    void cleanOrphanedConnections(const XObject * const sender,
        const LockPolicy lockPolicy = NeedToLock) {
        if (orphaned.load(std::memory_order_relaxed) && ref.loadAcquire() == 1)
            cleanOrphanedConnectionsImpl(sender, lockPolicy);
    }
    void cleanOrphanedConnectionsImpl(const XObject *sender, LockPolicy lockPolicy);

    ConnectionList &connectionsForSignal(int const signal) const{
        return signalVector.loadRelaxed()->at(signal);
    }

    void resizeSignalVector(uint size)
    {
        SignalVector *vector = this->signalVector.loadRelaxed();
        if (vector && vector->allocated > size)
            return;
        size = size + 7 & ~7;
        void *ptr = malloc(sizeof(SignalVector) + (size + 1) * sizeof(ConnectionList));
        const auto newVector = new (ptr) SignalVector;

        int start = -1;
        if (vector) {
            // not (yet) existing trait:
            // static_assert(std::is_relocatable_v<SignalVector>);
            // static_assert(std::is_relocatable_v<ConnectionList>);
            memcpy(newVector, vector,
                   sizeof(SignalVector) + (vector->allocated + 1) * sizeof(ConnectionList));
            start = vector->count();
        }
        for (int i = start; i < static_cast<int>(size); ++i){
            new (&newVector->at(i)) ConnectionList();
        }
        newVector->next = nullptr;
        newVector->allocated = size;

        signalVector.storeRelaxed(newVector);
        if (vector) {

            /* No ABA issue here: When adding a node, we only care about the list head, it doesn't
             * matter if the tail changes.
             */
            auto o{orphaned.load(std::memory_order_acquire)};
            do {
                vector->nextInOrphanList = o;
            } while (!orphaned.compare_exchange_strong(o, TaggedSignalVector(vector), std::memory_order_release));
        }
    }

    [[maybe_unused]] int signalVectorCount() const {
        return signalVector.loadAcquire() ? signalVector.loadRelaxed()->count() : -1;
    }

    static void deleteOrphaned(TaggedSignalVector o);
};

struct XObjectPrivate::Sender {
    Sender(XObject *receiver, XObject *sender, int signal, ConnectionData *receiverConnections)
            : receiver(receiver), sender(sender), signal(signal)
    {
        if (receiverConnections) {
            previous = receiverConnections->currentSender;
            receiverConnections->currentSender = this;
        }
    }

    ~Sender() {
        if (receiver){
            receiver->d_func()->connections.loadAcquire()->currentSender = previous;
        }
    }
    void receiverDeleted() {
        auto s {this};
        while (s) {
            s->receiver = nullptr;
            s = s->previous;
        }
    }
    Sender *previous {};
    XObject *receiver{},
    *sender{};
    int signal{};
};
X_DECLARE_TYPEINFO(XObjectPrivate::Sender, X_RELOCATABLE_TYPE);

class XObjectPrivate::XConnection final
        : public std::enable_shared_from_this<XConnection> {
    friend class XObject;
    X_DISABLE_COPY_MOVE(XConnection)
public:
    [[maybe_unused]] XObject * m_sender{};
    XAtomicPointer<XObject> m_receiver{};
    [[maybe_unused]] std::size_t m_signal_index{};
    XPrivate::XSignalSlotBase * m_slot_raw{};
    uint m_isSlotObject:1;

    explicit XConnection(XPrivate::XSignalSlotBase * const slot_base = {})
    :m_slot_raw(slot_base),m_isSlotObject{}
    {}

    ~XConnection() {
        std::cerr << FUNC_SIGNATURE << "\n";
        XPrivate::SlotObjUniquePtr slotObj{m_slot_raw};
    }
};

using XConnection_SPtr = std::shared_ptr<XObjectPrivate::XConnection>;
using XConnection_WPtr = std::weak_ptr<XObjectPrivate::XConnection>;
using XConnectionList = std::list<XConnection_SPtr>;
using XSendersList = std::list<XConnection_WPtr>;

class XObjectPrivate::XSignalVector final
        : public std::unordered_map<std::size_t ,XConnectionList> {
public:
    explicit XSignalVector() = default;
    ~XSignalVector() = default;
};

class XObjectPrivate::XConnectionData final {
public:
    explicit XConnectionData() = default;
    ~XConnectionData() {
        if(const auto v{m_signalVector.loadRelaxed()}) {
            delete v;
            m_signalVector.storeRelaxed({});
        }
    }

    XAtomicInt m_ref{};
    XAtomicPointer<XSignalVector> m_signalVector{};
    XSendersList m_senders{};
    XSender * m_currentSender{};

    inline void resizeSignalVector(std::size_t const signal_index) {

        auto v{m_signalVector.loadRelaxed()};
        if(!v){
            v = HelperClass::make_Unique<XSignalVector>().release();
            m_signalVector.storeRelaxed(v);
        }
        v->try_emplace(signal_index);
    }

    inline XConnectionList& connectionsForSignal(size_t const signal_index) const {
        return m_signalVector.loadRelaxed()->at(signal_index);
    }
};

class XObjectPrivate::XSender final {
public:
    [[maybe_unused]] XSender(XObject * const receiver, XObject * const sender, std::size_t const signal, XConnectionData * const receiverConnections)
            : m_receiver(receiver), m_sender(sender), m_signal(signal)
    {
        if (receiverConnections) {
            m_previous = receiverConnections->m_currentSender;
            receiverConnections->m_currentSender = this;
        }
    }

    ~XSender() {
        if (m_receiver){
            m_receiver->d_func()->m_connections.loadAcquire()->m_currentSender = m_previous;
        }
    }

    [[maybe_unused]] void receiverDeleted() {
        XSender *s = this;
        while (s) {
            s->m_previous = {};
            s = s->m_previous;
        }
    }

    XSender *m_previous{};
    [[maybe_unused]] XObject *m_receiver{},*m_sender{};
    [[maybe_unused]] std::size_t m_signal{};
};

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
