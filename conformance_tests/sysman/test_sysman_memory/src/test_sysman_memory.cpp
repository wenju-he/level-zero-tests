/*
 *
 * Copyright (C) 2020-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gtest/gtest.h"
#include "logging/logging.hpp"
#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include <condition_variable>
#include <thread>

namespace lzt = level_zero_tests;

#include <level_zero/zes_api.h>

namespace {

std::mutex mem_mutex;
std::condition_variable condition_variable;
uint32_t ready = 0;

#ifdef USE_ZESINIT
class MemoryModuleZesTest : public lzt::ZesSysmanCtsClass {};
#define MEMORY_TEST MemoryModuleZesTest
#else // USE_ZESINIT
class MemoryModuleTest : public lzt::SysmanCtsClass {};
#define MEMORY_TEST MemoryModuleTest
#endif // USE_ZESINIT

TEST_F(
    MEMORY_TEST,
    GivenComponentCountZeroWhenRetrievingSysmanHandlesThenNonZeroCountIsReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto mem_handles = lzt::get_mem_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }
  }
}

TEST_F(
    MEMORY_TEST,
    GivenComponentCountZeroWhenRetrievingSysmanHandlesThenNotNullMemoryHandlesAreReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto mem_handles = lzt::get_mem_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    ASSERT_EQ(mem_handles.size(), count);
    for (auto mem_handle : mem_handles) {
      EXPECT_NE(nullptr, mem_handle);
    }
  }
}

TEST_F(
    MEMORY_TEST,
    GivenInvalidComponentCountWhenRetrievingSysmanHandlesThenActualComponentCountIsUpdated) {
  for (auto device : devices) {
    uint32_t actual_count = 0;
    lzt::get_mem_handles(device, actual_count);
    if (actual_count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    uint32_t test_count = actual_count + 1;
    lzt::get_mem_handles(device, test_count);
    EXPECT_EQ(test_count, actual_count);
  }
}

TEST_F(
    MEMORY_TEST,
    GivenValidComponentCountWhenCallingApiTwiceThenSimilarMemHandlesReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto mem_handles_initial = lzt::get_mem_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto mem_handle : mem_handles_initial) {
      EXPECT_NE(nullptr, mem_handle);
    }

    count = 0;
    auto mem_handles_later = lzt::get_mem_handles(device, count);
    for (auto mem_handle : mem_handles_later) {
      EXPECT_NE(nullptr, mem_handle);
    }
    EXPECT_EQ(mem_handles_initial, mem_handles_later);
  }
}

TEST_F(
    MEMORY_TEST,
    GivenValidMemHandleWhenRetrievingMemPropertiesThenValidPropertiesAreReturned) {
  for (auto device : devices) {
    auto deviceProperties = lzt::get_sysman_device_properties(device);
    uint32_t count = 0;
    auto mem_handles = lzt::get_mem_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto mem_handle : mem_handles) {
      ASSERT_NE(nullptr, mem_handle);
      auto properties = lzt::get_mem_properties(mem_handle);
      if (properties.onSubdevice) {
        EXPECT_LT(properties.subdeviceId, deviceProperties.numSubdevices);
      }
      EXPECT_LT(properties.physicalSize, UINT64_MAX);
      EXPECT_GE(properties.location, ZES_MEM_LOC_SYSTEM);
      EXPECT_LE(properties.location, ZES_MEM_LOC_DEVICE);
      EXPECT_LE(properties.busWidth, INT32_MAX);
      EXPECT_GE(properties.busWidth, -1);
      EXPECT_NE(properties.busWidth, 0);
      EXPECT_LE(properties.numChannels, INT32_MAX);
      EXPECT_GE(properties.numChannels, -1);
      EXPECT_NE(properties.numChannels, 0);
    }
  }
}

TEST_F(
    MEMORY_TEST,
    GivenValidMemHandleWhenRetrievingMemPropertiesThenExpectSamePropertiesReturnedTwice) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto mem_handles = lzt::get_mem_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto mem_handle : mem_handles) {
      EXPECT_NE(nullptr, mem_handle);
      auto properties_initial = lzt::get_mem_properties(mem_handle);
      auto properties_later = lzt::get_mem_properties(mem_handle);
      EXPECT_EQ(properties_initial.type, properties_later.type);
      EXPECT_EQ(properties_initial.onSubdevice, properties_later.onSubdevice);
      if (properties_initial.onSubdevice && properties_later.onSubdevice) {
        EXPECT_EQ(properties_initial.subdeviceId, properties_later.subdeviceId);
      }
      EXPECT_EQ(properties_initial.physicalSize, properties_later.physicalSize);
      EXPECT_EQ(properties_initial.location, properties_later.location);
      EXPECT_EQ(properties_initial.busWidth, properties_later.busWidth);
      EXPECT_EQ(properties_initial.numChannels, properties_later.numChannels);
    }
  }
}

TEST_F(
    MEMORY_TEST,
    GivenValidMemHandleWhenRetrievingMemBandWidthThenValidBandWidthCountersAreReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto mem_handles = lzt::get_mem_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto mem_handle : mem_handles) {
      ASSERT_NE(nullptr, mem_handle);
      auto bandwidth = lzt::get_mem_bandwidth(mem_handle);
      EXPECT_LT(bandwidth.readCounter, UINT64_MAX);
      EXPECT_LT(bandwidth.writeCounter, UINT64_MAX);
      EXPECT_LT(bandwidth.maxBandwidth, UINT64_MAX);
      EXPECT_LT(bandwidth.timestamp, UINT64_MAX);
    }
  }
}

TEST_F(MEMORY_TEST,
       GivenValidMemHandleWhenRetrievingMemStateThenValidStateIsReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto mem_handles = lzt::get_mem_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto mem_handle : mem_handles) {
      ASSERT_NE(nullptr, mem_handle);
      auto state = lzt::get_mem_state(mem_handle);
      EXPECT_GE(state.health, ZES_MEM_HEALTH_UNKNOWN);
      EXPECT_LE(state.health, ZES_MEM_HEALTH_REPLACE);
      auto properties = lzt::get_mem_properties(mem_handle);
      if (properties.physicalSize != 0) {
        EXPECT_LE(state.size, properties.physicalSize);
      }
      EXPECT_LE(state.free, state.size);
    }
  }
}

uint64_t get_free_memory_state(ze_device_handle_t device) {
  uint32_t count = 0;
  uint64_t total_free_memory = 0;
  uint64_t total_alloc = 0;

  std::vector<zes_mem_handle_t> mem_handles =
      lzt::get_mem_handles(device, count);
  if (count == 0) {
    std::cout << "No handles found: "
              << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
  }
  for (zes_mem_handle_t mem_handle : mem_handles) {
    auto state = lzt::get_mem_state(mem_handle);
    total_free_memory += state.free;
    total_alloc += state.size;
  }

  return total_free_memory;
}

#ifdef USE_ZESINIT
bool is_uuids_equal(uint8_t *uuid1, uint8_t *uuid2) {
  for (uint32_t i = 0; i < ZE_MAX_UUID_SIZE; i++) {
    if (uuid1[i] != uuid2[i]) {
      return false;
    }
  }
  return true;
}
ze_device_handle_t get_core_device_by_uuid(uint8_t *uuid) {
  lzt::initialize_core();
  auto driver = lzt::zeDevice::get_instance()->get_driver();
  auto core_devices = lzt::get_ze_devices(driver);
  for (auto device : core_devices) {
    auto device_properties = lzt::get_device_properties(device);
    if (is_uuids_equal(uuid, device_properties.uuid.id)) {
      return device;
    }
  }
  return nullptr;
}
#endif // USE_ZESINIT

TEST_F(
    MEMORY_TEST,
    GivenValidMemHandleWhenAllocatingMemoryUptoMaxCapacityThenOutOfDeviceMemoryErrorIsReturned) {

  for (ze_device_handle_t device : devices) {
    uint32_t count = 0;
    uint64_t total_free = 0;
    std::vector<void *> vec_ptr;
#ifdef USE_ZESINIT
    auto sysman_device_properties = lzt::get_sysman_device_properties(device);
    ze_device_handle_t core_device =
        get_core_device_by_uuid(sysman_device_properties.core.uuid.id);
    EXPECT_NE(core_device, nullptr);
    ze_device_properties_t deviceProperties = {
        ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES, nullptr};
    zeDeviceGetProperties(core_device, &deviceProperties);
    std::cout << "test device name " << deviceProperties.name << " uuid "
              << lzt::to_string(deviceProperties.uuid);
#else  // USE_ZESINIT
    ze_device_properties_t deviceProperties = {
        ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES, nullptr};
    zeDeviceGetProperties(device, &deviceProperties);
    std::cout << "test device name " << deviceProperties.name << " uuid "
              << lzt::to_string(deviceProperties.uuid);
#endif // USE_ZESINIT
    if (deviceProperties.flags & ZE_DEVICE_PROPERTY_FLAG_SUBDEVICE) {
      std::cout << "test subdevice id " << deviceProperties.subdeviceId;
    } else {
      std::cout << "test device is a root device" << std::endl;
    }

    uint64_t max_alloc_size = deviceProperties.maxMemAllocSize;
    uint64_t alloc_size = ((uint64_t)4 * 1024 * 1024 * 1024);
    if (max_alloc_size < alloc_size) {
      alloc_size = max_alloc_size;
    }
    uint64_t free_memory = get_free_memory_state(device);
    void *ze_buf = nullptr;
    ze_result_t result = ZE_RESULT_SUCCESS;
    uint8_t pattern = 0xAB;

    auto local_mem = lzt::allocate_host_memory(alloc_size);
    EXPECT_NE(nullptr, local_mem);
    std::memset(local_mem, pattern, alloc_size);
    result = zeContextMakeMemoryResident(lzt::get_default_context(), device,
                                         local_mem, alloc_size);
    EXPECT_EQ(result, ZE_RESULT_SUCCESS);

    uint64_t cur_mem_alloc_size = 0;
    do {
      ze_buf = nullptr;
      if (alloc_size <= free_memory) {
        cur_mem_alloc_size = alloc_size;
      } else {
        cur_mem_alloc_size = free_memory;
      }
      auto ze_buf = lzt::allocate_device_memory(cur_mem_alloc_size);
      EXPECT_NE(nullptr, ze_buf);
      if (ze_buf != nullptr) {
        vec_ptr.push_back(ze_buf);
      } else {
        std::cout << "Memory Allocation Failed..." << std::endl;
        break;
      }

      result = zeContextMakeMemoryResident(lzt::get_default_context(), device,
                                           ze_buf, cur_mem_alloc_size);
      free_memory = get_free_memory_state(device);
    } while ((result != ZE_RESULT_ERROR_OUT_OF_DEVICE_MEMORY) &&
             (free_memory > 0));

    EXPECT_EQ(ZE_RESULT_ERROR_OUT_OF_DEVICE_MEMORY, result);

    // Free allocated device memory
    for (int i = 0; i < vec_ptr.size(); i++) {
      zeMemFree(lzt::get_default_context(), vec_ptr[i]);
    }
    vec_ptr.clear();
    lzt::free_memory(local_mem);
  }
}

void getMemoryState(ze_device_handle_t device) {
  uint32_t count = 0;
  std::vector<zes_mem_handle_t> mem_handles =
      lzt::get_mem_handles(device, count);
  if (count == 0) {
    FAIL() << "No handles found: "
           << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
  }
  std::unique_lock<std::mutex> lock(mem_mutex);
  ready++;
  condition_variable.notify_all();
  condition_variable.wait(lock, [] { return ready == 2; });
  for (auto mem_handle : mem_handles) {
    ASSERT_NE(nullptr, mem_handle);
    lzt::get_mem_state(mem_handle);
  }
}

void getRasState(ze_device_handle_t device) {
  uint32_t count = 0;
  std::vector<zes_ras_handle_t> ras_handles =
      lzt::get_ras_handles(device, count);
  if (count == 0) {
    FAIL() << "No handles found: "
           << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
  }
  std::unique_lock<std::mutex> lock(mem_mutex);
  ready++;
  condition_variable.notify_all();
  condition_variable.wait(lock, [] { return ready == 2; });
  for (auto ras_handle : ras_handles) {
    ASSERT_NE(nullptr, ras_handle);
    ze_bool_t clear = 0;
    lzt::get_ras_state(ras_handle, clear);
  }
}

TEST_F(
    MEMORY_TEST,
    GivenValidMemoryAndRasHandlesWhenGettingMemoryGetStateAndRasGetStateFromDifferentThreadsThenExpectBothToReturnSucess) {
  for (auto device : devices) {
    std::thread rasThread(getRasState, device);
    std::thread memoryThread(getMemoryState, device);
    rasThread.join();
    memoryThread.join();
  }
}
} // namespace
