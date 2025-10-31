#include <departmentnode.hpp>
#include <employeenode.hpp>

int main() {

    DepartmentNode root{"XXX公司"};

    auto const FinanceDepartment
    { std::make_shared<DepartmentNode>("财务部") };

    auto const DevelopmentDepartment
    { std::make_shared<DepartmentNode>("开发部") };

    auto const MuSir
    { std::make_shared<EmployeeNode>("mu") };

    auto const MissWang
    { std::make_shared<EmployeeNode>("missWang") };

    root.addChild(FinanceDepartment);
    root.addChild(DevelopmentDepartment);

    DevelopmentDepartment->addChild(MuSir);
    FinanceDepartment->addChild(MissWang);

    root.printInfo(0);

    return 0;
}
