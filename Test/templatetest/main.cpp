#include <XContainerHelper/xcontainerhelper.hpp>

int main()
{
    std::vector<int> v,v1{};
    constexpr int arr[]{0,1,2,3,4,5,6,7,8,9};

    XUtils::append(v1,std::initializer_list{0,1,2,3,4,5,6,7,8,9});
    XUtils::append(v1,0,1,2,3,4,5,6,7,8,9);
    XUtils::append(v,v1);
    XUtils::append(v1,arr,std::size(arr));

    return 0;
}
