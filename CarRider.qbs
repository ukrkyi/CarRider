import qbs

CppApplication {

    targetName: "out.elf"

    consoleApplication: true
    files: [
        "CMSIS/inc/cmsis_compiler.h",
        "CMSIS/inc/cmsis_gcc.h",
        "CMSIS/inc/cmsis_version.h",
        "CMSIS/inc/core_cm4.h",
        "CMSIS/inc/mpu_armv7.h",
        "CMSIS/inc/stm32f401xc.h",
        "CMSIS/inc/stm32f4xx.h",
        "CMSIS/inc/system_stm32f4xx.h",
        "CMSIS/src/startup_stm32f401xc.s",
        "CMSIS/src/system_stm32f4xx.c",
        "FreeRTOS/inc/FreeRTOS.h",
        "FreeRTOS/inc/StackMacros.h",
        "FreeRTOS/inc/atomic.h",
        "FreeRTOS/inc/croutine.h",
        "FreeRTOS/inc/deprecated_definitions.h",
        "FreeRTOS/inc/event_groups.h",
        "FreeRTOS/inc/list.h",
        "FreeRTOS/inc/message_buffer.h",
        "FreeRTOS/inc/mpu_prototypes.h",
        "FreeRTOS/inc/mpu_wrappers.h",
        "FreeRTOS/inc/portable.h",
        "FreeRTOS/inc/projdefs.h",
        "FreeRTOS/inc/queue.h",
        "FreeRTOS/inc/semphr.h",
        "FreeRTOS/inc/stack_macros.h",
        "FreeRTOS/inc/stream_buffer.h",
        "FreeRTOS/inc/task.h",
        "FreeRTOS/inc/timers.h",
        "FreeRTOS/portable/GCC_ARM_CM4F/port.c",
        "FreeRTOS/portable/GCC_ARM_CM4F/portmacro.h",
        "FreeRTOS/src/FreeRTOS-openocd.c",
        "FreeRTOS/src/event_groups.c",
        "FreeRTOS/src/list.c",
        "FreeRTOS/src/queue.c",
        "FreeRTOS/src/tasks.c",
        "FreeRTOS/src/timers.c",
        "HAL/inc/Legacy/stm32_hal_legacy.h",
        "HAL/inc/stm32_assert.h",
        "HAL/inc/stm32f4xx_hal.h",
        "HAL/inc/stm32f4xx_hal_cortex.h",
        "HAL/inc/stm32f4xx_hal_def.h",
        "HAL/inc/stm32f4xx_hal_dma.h",
        "HAL/inc/stm32f4xx_hal_dma_ex.h",
        "HAL/inc/stm32f4xx_hal_flash.h",
        "HAL/inc/stm32f4xx_hal_flash_ex.h",
        "HAL/inc/stm32f4xx_hal_flash_ramfunc.h",
        "HAL/inc/stm32f4xx_hal_gpio.h",
        "HAL/inc/stm32f4xx_hal_gpio_ex.h",
        "HAL/inc/stm32f4xx_hal_pwr.h",
        "HAL/inc/stm32f4xx_hal_pwr_ex.h",
        "HAL/inc/stm32f4xx_hal_rcc.h",
        "HAL/inc/stm32f4xx_hal_rcc_ex.h",
        "HAL/inc/stm32f4xx_hal_tim.h",
        "HAL/inc/stm32f4xx_hal_tim_ex.h",
        "HAL/inc/stm32f4xx_ll_bus.h",
        "HAL/inc/stm32f4xx_ll_dma.h",
        "HAL/inc/stm32f4xx_ll_rcc.h",
        "HAL/inc/stm32f4xx_ll_usart.h",
        "HAL/src/stm32f4xx_hal_gpio.c",
        "HAL/src/stm32f4xx_hal_rcc.c",
        "HAL/src/stm32f4xx_hal_tim.c",
        "HAL/src/stm32f4xx_ll_rcc.c",
        "HAL/src/stm32f4xx_ll_usart.c",
        "STM32F401CCUx_FLASH.ld",
        "inc/FreeRTOSConfig.h",
        "inc/buffer.h",
        "inc/led.h",
        "inc/motor_dc.h",
        "inc/pwm.h",
        "inc/stm32f4xx_hal_conf.h",
        "inc/system.h",
        "inc/uart.h",
        "inc/ultrasonic.h",
        "interrupts.cpp",
        "main.cpp",
        "os_drivers/inc/eventgroup.h",
        "os_drivers/inc/mutex.h",
        "os_drivers/inc/queue.hpp",
        "os_drivers/inc/semaphore.h",
        "os_drivers/src/eventgroup.cpp",
        "os_drivers/src/mutex.cpp",
        "os_drivers/src/semaphore.cpp",
        "src/led.cpp",
        "src/motor_dc.cpp",
        "src/pwm.cpp",
        "src/system.c",
        "src/uart.cpp",
        "src/ultrasonic.cpp",
    ]

    FileTagger {
        patterns: "*.ld"
        fileTags: ["linkerscript"]
    }

    Properties {
        condition: qbs.buildVariant === "debug"

        cpp.commonCompilerFlags: [
            "-g3",
            "-ffunction-sections",
            "-fdata-sections"
        ]
        cpp.debugInformation: true
        qbs.optimization: "none"
    }

    Properties {
        condition: qbs.buildVariant === "release"

        cpp.commonCompilerFlags: [
            "-flto",
            "-ffunction-sections",
            "-fdata-sections",
            "-DNDEBUG"
        ]
        //cpp.defines: ['NDEBUG'] can't use here bc it overwrites other defines
        cpp.debugInformation: false
        qbs.optimization: "small"
    }

    cpp.defines: ['STM32F401xC', 'USE_HAL_DRIVER', 'USE_FULL_LL_DRIVER']
    cpp.includePaths: ["inc", "os_drivers/inc"]
    cpp.systemIncludePaths: ["CMSIS/inc", "HAL/inc", "FreeRTOS/inc", "FreeRTOS/portable/GCC_ARM_CM4F"]

    cpp.warningLevel: 'all'
    cpp.positionIndependentCode: false

    cpp.generateLinkerMapFile: true

    cpp.driverFlags: [ // Загальні прапорці для всіх етапів компіляції
        "-mcpu=cortex-m4",
        "-mthumb",
        "-mfloat-abi=hard",
    ]

    cpp.linkerFlags: [
        "--gc-sections",
        "--print-memory-usage",
        "--undefined=uxTopUsedPriority"
    ]

    cpp.driverLinkerFlags: [
        //"--specs=nano.specs",
        "-specs=nosys.specs"
    ]

    Group {     // Properties for the produced executable
        fileTagsFilter: "application"
        qbs.install: true
        qbs.installDir: "bin"
    }
}
