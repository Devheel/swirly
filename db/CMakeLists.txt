# The Restful Matching-Engine.
# Copyright (C) 2013, 2018 Swirly Cloud Limited.
#
# This program is free software; you can redistribute it and/or modify it under the terms of the
# GNU General Public License as published by the Free Software Foundation; either version 2 of the
# License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
# even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License along with this program; if
# not, write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
# 02110-1301, USA.

include(AddFileDependencies)

set(dbfiles)
if(SQLITE3_EXECUTABLE)

  message(STATUS "Sqlite3 found: ${SQLITE3_EXECUTABLE}")

  set(names
    coal
    coffee
    corp
    forex
  )

  foreach(name ${names})

    set(dbfile "${PROJECT_BINARY_DIR}/db/${name}.db")

    add_custom_command(
      OUTPUT "${dbfile}"
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
      COMMAND ${CMAKE_COMMAND} -E remove "${dbfile}"
      COMMAND "${SQLITE3_EXECUTABLE}" "${dbfile}" <schema.sql
      COMMAND "${SQLITE3_EXECUTABLE}" "${dbfile}" <${name}.sql
      DEPENDS schema.sql ${name}.sql)

    set(dbfiles ${dbfiles} "${dbfile}")
  endforeach()

endif()

add_custom_target(db ALL DEPENDS ${dbfiles})

set(sqlfiles
  coal.sql
  coffee.sql
  corp.sql
  forex.sql
  schema.sql
  test.sql)

install(FILES ${sqlfiles} DESTINATION db COMPONENT config)
install(FILES ${dbfiles} DESTINATION var COMPONENT config)
