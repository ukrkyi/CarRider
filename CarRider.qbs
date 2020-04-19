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
        "HAL/inc/Legacy/stm32_hal_legacy.h",
        "HAL/inc/stm32f4xx_hal.h",
        "HAL/inc/stm32f4xx_hal_conf.h",
        "HAL/inc/stm32f4xx_hal_def.h",
        "HAL/inc/stm32f4xx_hal_dma.h",
        "HAL/inc/stm32f4xx_hal_dma_ex.h",
        "HAL/inc/stm32f4xx_hal_gpio.h",
        "HAL/inc/stm32f4xx_hal_gpio_ex.h",
        "HAL/inc/stm32f4xx_hal_rcc.h",
        "HAL/inc/stm32f4xx_hal_rcc_ex.h",
        "HAL/inc/stm32f4xx_hal_tim.h",
        "HAL/inc/stm32f4xx_hal_tim_ex.h",
        "HAL/src/stm32f4xx_hal_gpio.c",
        "HAL/src/stm32f4xx_hal_tim.c",
        "STM32F401CCUx_FLASH.ld",
        "inc/led.h",
        "inc/motor_dc.h",
        "inc/pwm.h",
        "inc/ultrasonic.h",
        "main.cpp",
        "src/led.cpp",
        "src/motor_dc.cpp",
        "src/pwm.cpp",
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

    cpp.defines: ['STM32F401xC', 'USE_HAL_DRIVER']
    cpp.includePaths: ["inc"]
    cpp.systemIncludePaths: ["CMSIS/inc", "HAL/inc"]

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
