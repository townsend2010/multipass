# Copyright © 2017-2018 Canonical Ltd.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 3 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# Authored by: Alberto Aguirre <alberto.aguirre@canonical.com>

list(APPEND BACKEND_TESTS test_qemu_backend.cpp)

add_executable(qemu-system-x86_64
        mock_qemu_system.cpp)

set_target_properties(qemu-system-x86_64
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/mocks"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin/mocks"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin/mocks")

set(MULTIPASS_GMOCK_DIR ${CMAKE_SOURCE_DIR}/3rd-party/grpc/third_party/googletest/googlemock)
set(MULTIPASS_GTEST_DIR ${CMAKE_SOURCE_DIR}/3rd-party/grpc/third_party/googletest/googletest)

add_executable(multipass_tests
  file_operations.cpp
  main.cpp
  mock_scp.cpp
  mock_sftpserver.cpp
  mock_ssh.cpp
  mock_ssh_client.cpp
  path.cpp
  temp_dir.cpp
  temp_file.cpp
  test_cli_client.cpp
  test_client_cert_store.cpp
  test_cloud_init_iso.cpp
  test_daemon.cpp
  test_format_utils.cpp
  test_output_formatter.cpp
  test_image_vault.cpp
  test_ip_address_pool.cpp
  test_metrics_provider.cpp
  test_petname.cpp
  test_simple_streams_index.cpp
  test_simple_streams_manifest.cpp
  test_scp_client.cpp
  test_sftpserver.cpp
  test_ssl_cert_provider.cpp
  test_sshfsmount.cpp
  test_ssh_client.cpp
  test_ssh_key_provider.cpp
  test_ssh_process.cpp
  test_ssh_session.cpp
  test_ubuntu_image_host.cpp
  test_utils.cpp

  ${BACKEND_TESTS}

  ${MULTIPASS_GMOCK_DIR}/src/gmock-all.cc
  ${MULTIPASS_GTEST_DIR}/src/gtest-all.cc
)

target_include_directories(multipass_tests
  PRIVATE ${CMAKE_SOURCE_DIR}
  PRIVATE ${MULTIPASS_GTEST_DIR}
  PRIVATE ${MULTIPASS_GTEST_DIR}/include
  PRIVATE ${MULTIPASS_GMOCK_DIR}
  PRIVATE ${MULTIPASS_GMOCK_DIR}/include
)

include(c_mock_defines.cmake)
target_compile_definitions(ssh_test PRIVATE
  ${c_mock_defines})
target_compile_definitions(sshfs_mount_test PRIVATE
  ${c_mock_defines})
target_compile_definitions(scp_test PRIVATE
  ${c_mock_defines})
target_compile_definitions(ssh_client_test PRIVATE
  ${c_mock_defines})

target_link_libraries(multipass_tests
  cert
  client
  daemon
  ip_address_pool
  iso
  metrics
  petname
  simplestreams
  scp_test
  ssh_test
  ssh_client_test
  sshfs_mount_test
  utils
  # 3rd-party
  premock
  yaml
)

add_test(NAME multipass_tests
  COMMAND multipass_tests
)

file(COPY test_data DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
