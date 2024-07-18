AC_PREREQ(2.61)

# package version number (not shared library version)
# odd micro numbers indicate in-progress development
# even micro numbers indicate released versions
m4_define(passerelle_modbus_version_major, 0)
m4_define(passerelle_modbus_version_minor, 1)
m4_define(passerelle_modbus_version_micro, 0)

m4_define([passerelle_modbus_version],
          [passerelle_modbus_version_major.passerelle_modbus_version_minor.passerelle_modbus_version_micro])
m4_define([passerelle_modbus_api_version],
          [passerelle_modbus_version_major.passerelle_modbus_version_minor])

AC_INIT(passerelle_modbus, passerelle_modbus_version)
AM_INIT_AUTOMAKE(passerelle_modbus,passerelle_modbus_version)
AM_INIT_AUTOMAKE([foreign])
AC_CONFIG_HEADERS(config.h)

# Checks for programs.
AC_PROG_LIBTOOL
AM_PROG_CC_C_O
AC_PROG_INSTALL

AC_SUBST(passerelle_modbus_CFLAGS)
AC_SUBST(passerelle_modbus_LIBS)

AC_OUTPUT([
Makefile
program/Makefile
])

echo ""
echo "   passerelle_modbus $VERSION"
echo "   ====================="
echo ""
echo "   To build the project, run \"make\""
echo ""
