vcpkg_minimum_required(VERSION 2022-10-12)
set(VERSION 6.3.0)
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO user-sys-pkg/gmp
    REF dummy
    SHA512 0
    HEAD_REF dummy
)
file(INSTALL /usr/include/gmp.h DESTINATION "${CURRENT_PACKAGES_DIR}/include")
file(INSTALL /usr/include/gmpxx.h DESTINATION "${CURRENT_PACKAGES_DIR}/include")
file(INSTALL /usr/lib/libgmp.so DESTINATION "${CURRENT_PACKAGES_DIR}/lib")
file(INSTALL /usr/lib/libgmpxx.so DESTINATION "${CURRENT_PACKAGES_DIR}/lib")
file(WRITE "${CURRENT_PACKAGES_DIR}/share/gmp/copyright" "System GMP")
set(VCPKG_POLICY_EMPTY_INSTALLED_FILES  AUTOMATION)
