
tup.include('build.lua')

-- Switch between board versions
boardversion = tup.getconfig("BOARD_VERSION")
if boardversion == "v3.1" then
    boarddir = 'Board/v3' -- currently all platform code is in the same v3.3 directory
    FLAGS += "-DHW_VERSION_MAJOR=3 -DHW_VERSION_MINOR=1"
    FLAGS += "-DHW_VERSION_VOLTAGE=24"
elseif boardversion == "v3.2" then
    boarddir = 'Board/v3'
    FLAGS += "-DHW_VERSION_MAJOR=3 -DHW_VERSION_MINOR=2"
    FLAGS += "-DHW_VERSION_VOLTAGE=24"
elseif boardversion == "v3.3" then
    boarddir = 'Board/v3'
    FLAGS += "-DHW_VERSION_MAJOR=3 -DHW_VERSION_MINOR=3"
    FLAGS += "-DHW_VERSION_VOLTAGE=24"
elseif boardversion == "v3.4-24V" then
    boarddir = 'Board/v3'
    FLAGS += "-DHW_VERSION_MAJOR=3 -DHW_VERSION_MINOR=4"
    FLAGS += "-DHW_VERSION_VOLTAGE=24"
elseif boardversion == "v3.4-48V" then
    boarddir = 'Board/v3'
    FLAGS += "-DHW_VERSION_MAJOR=3 -DHW_VERSION_MINOR=4"
    FLAGS += "-DHW_VERSION_VOLTAGE=48"
elseif boardversion == "v3.5-24V" then
    boarddir = 'Board/v3'
    FLAGS += "-DHW_VERSION_MAJOR=3 -DHW_VERSION_MINOR=5"
    FLAGS += "-DHW_VERSION_VOLTAGE=24"
elseif boardversion == "v3.5-48V" then
    boarddir = 'Board/v3'
    FLAGS += "-DHW_VERSION_MAJOR=3 -DHW_VERSION_MINOR=5"
    FLAGS += "-DHW_VERSION_VOLTAGE=48"
elseif boardversion == "" then
    error("board version not specified - take a look at tup.config.default")
else
    error("unknown board version "..boardversion)
end
buildsuffix = boardversion

-- USB I/O settings
if tup.getconfig("USB_PROTOCOL") == "native" or tup.getconfig("USB_PROTOCOL") == "" then
    FLAGS += "-DUSB_PROTOCOL_NATIVE"
elseif tup.getconfig("USB_PROTOCOL") == "native-stream" then
    FLAGS += "-DUSB_PROTOCOL_NATIVE_STREAM_BASED"
elseif tup.getconfig("USB_PROTOCOL") == "stdout" then
    FLAGS += "-DUSB_PROTOCOL_STDOUT"
elseif tup.getconfig("USB_PROTOCOL") == "none" then
    FLAGS += "-DUSB_PROTOCOL_NONE"
else
    error("unknown USB protocol")
end

-- UART I/O settings
if tup.getconfig("UART_PROTOCOL") == "native" then
    FLAGS += "-DUART_PROTOCOL_NATIVE"
elseif tup.getconfig("UART_PROTOCOL") == "ascii" or tup.getconfig("UART_PROTOCOL") == "" then
    FLAGS += "-DUART_PROTOCOL_ASCII"
elseif tup.getconfig("UART_PROTOCOL") == "stdout" then
    FLAGS += "-DUART_PROTOCOL_STDOUT"
elseif tup.getconfig("UART_PROTOCOL") == "none" then
    FLAGS += "-DUART_PROTOCOL_NONE"
else
    error("unknown UART protocol "..tup.getconfig("UART_PROTOCOL"))
end

-- GPIO settings
if tup.getconfig("STEP_DIR") == "y" then
    if tup.getconfig("UART_PROTOCOL") == "none" then
        FLAGS += "-DUSE_GPIO_MODE_STEP_DIR"
    else
        error("Step/dir mode conflicts with UART. Set CONFIG_UART_PROTOCOL to none.")
    end
end

-- Compiler settings
if tup.getconfig("STRICT") == "true" then
    FLAGS += '-Werror'
end


-- C-specific flags
FLAGS += '-D__weak="__attribute__((weak))"'
FLAGS += '-D__packed="__attribute__((__packed__))"'
FLAGS += '-DUSE_HAL_DRIVER'
FLAGS += '-DSTM32F405xx'

FLAGS += '-mthumb'
FLAGS += '-mcpu=cortex-m4'
FLAGS += '-mfpu=fpv4-sp-d16'
FLAGS += '-mfloat-abi=hard'
FLAGS += { '-Wall', '-Wdouble-promotion', '-Wfloat-conversion', '-fdata-sections', '-ffunction-sections'}

-- debug build
FLAGS += '-g -gdwarf-2'


-- linker flags
LDFLAGS += '-T'..boarddir..'/STM32F405RGTx_FLASH.ld'
LDFLAGS += '-L'..boarddir..'/Drivers/CMSIS/Lib' -- lib dir
LDFLAGS += '-lc -lm -lnosys -larm_cortexM4lf_math' -- libs
LDFLAGS += '-mthumb -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard -specs=nosys.specs -specs=nano.specs -u _printf_float -u _scanf_float -Wl,--cref -Wl,--gc-sections'
LDFLAGS += '-Wl,--undefined=uxTopUsedPriority'


-- common flags for ASM, C and C++
OPT += '-Og'
-- OPT += '-O0'
OPT += '-ffast-math -fno-finite-math-only'
tup.append_table(FLAGS, OPT)
tup.append_table(LDFLAGS, OPT)

toolchain = GCCToolchain('arm-none-eabi-', 'build', FLAGS, LDFLAGS)


-- Load list of source files Makefile that was autogenerated by CubeMX
vars = parse_makefile_vars(boarddir..'/Makefile')
all_stm_sources = (vars['C_SOURCES'] or '')..' '..(vars['CPP_SOURCES'] or '')..' '..(vars['ASM_SOURCES'] or '')
for src in string.gmatch(all_stm_sources, "%S+") do
    stm_sources += boarddir..'/'..src
end
for src in string.gmatch(vars['C_INCLUDES'] or '', "%S+") do
    stm_includes += boarddir..'/'..string.sub(src, 3, -1) -- remove "-I" from each include path
end

-- TODO: cleaner separation of the platform code and the rest
stm_includes += '.'
stm_includes += 'Drivers/DRV8301'
stm_sources += boarddir..'/Src/syscalls.c'
build{
    name='stm_platform',
    type='objects',
    toolchains={toolchain},
    packages={},
    sources=stm_sources,
    includes=stm_includes
}

tup.frule{
    command='python ../tools/odrive/version.py --output %o',
    outputs={'build/version.h'}
}

tup.frule{
    inputs= tup.glob('communication/networking/wwwroot/*'),
    command= './communication/networking/makefsdata.pl communication/networking/wwwroot %o',
    outputs={'build/fsdata.c'}
}


-- tup.glob can't resolve communication/**/*.c, 
-- so we have to make an explicit list. 
network_sources = {
    'build/fsdata.c',
    'communication/networking/network_core.c',
    'communication/networking/url_handlers.c',
    'communication/networking/lrndis/rndis-stm32/usbd_rndis_core.c',
    'communication/networking/lrndis/dhcp-server/dhserver.c',
    'communication/networking/lrndis/dns-server/dnserver.c',
    'communication/networking/lrndis/lwip-1.4.1/apps/rtp/rtp.c',
    'communication/networking/lrndis/lwip-1.4.1/apps/netbios/netbios.c',
    'communication/networking/lrndis/lwip-1.4.1/apps/chargen/chargen.c',
    'communication/networking/lrndis/lwip-1.4.1/apps/socket_examples/socket_examples.c',
    'communication/networking/lrndis/lwip-1.4.1/apps/tcpecho_raw/echo.c',
    'communication/networking/lrndis/lwip-1.4.1/apps/snmp_private_mib/lwip_prvmib.c',
    'communication/networking/lrndis/lwip-1.4.1/apps/udpecho/udpecho.c',
    'communication/networking/lrndis/lwip-1.4.1/apps/netio/netio.c',
    'communication/networking/lrndis/lwip-1.4.1/apps/httpserver/httpserver-netconn.c',
    'communication/networking/lrndis/lwip-1.4.1/apps/httpserver_raw/httpd.c',
    'communication/networking/lrndis/lwip-1.4.1/apps/httpserver_raw/fs.c',
    'communication/networking/lrndis/lwip-1.4.1/apps/tcpecho/tcpecho.c',
    'communication/networking/lrndis/lwip-1.4.1/apps/shell/shell.c',
    'communication/networking/lrndis/lwip-1.4.1/apps/sntp/sntp.c',
    'communication/networking/lrndis/lwip-1.4.1/src/api/tcpip.c',
    'communication/networking/lrndis/lwip-1.4.1/src/api/err.c',
    'communication/networking/lrndis/lwip-1.4.1/src/api/api_lib.c',
    'communication/networking/lrndis/lwip-1.4.1/src/api/netifapi.c',
    'communication/networking/lrndis/lwip-1.4.1/src/api/sockets.c',
    'communication/networking/lrndis/lwip-1.4.1/src/api/netdb.c',
    'communication/networking/lrndis/lwip-1.4.1/src/api/netbuf.c',
    'communication/networking/lrndis/lwip-1.4.1/src/api/api_msg.c',
    'communication/networking/lrndis/lwip-1.4.1/src/netif/slipif.c',
    'communication/networking/lrndis/lwip-1.4.1/src/netif/ppp/lcp.c',
    'communication/networking/lrndis/lwip-1.4.1/src/netif/ppp/vj.c',
    'communication/networking/lrndis/lwip-1.4.1/src/netif/ppp/md5.c',
    'communication/networking/lrndis/lwip-1.4.1/src/netif/ppp/pap.c',
    'communication/networking/lrndis/lwip-1.4.1/src/netif/ppp/chpms.c',
    'communication/networking/lrndis/lwip-1.4.1/src/netif/ppp/fsm.c',
    'communication/networking/lrndis/lwip-1.4.1/src/netif/ppp/ipcp.c',
    'communication/networking/lrndis/lwip-1.4.1/src/netif/ppp/auth.c',
    'communication/networking/lrndis/lwip-1.4.1/src/netif/ppp/magic.c',
    'communication/networking/lrndis/lwip-1.4.1/src/netif/ppp/chap.c',
    'communication/networking/lrndis/lwip-1.4.1/src/netif/ppp/ppp_oe.c',
    'communication/networking/lrndis/lwip-1.4.1/src/netif/ppp/ppp.c',
    'communication/networking/lrndis/lwip-1.4.1/src/netif/ppp/randm.c',
    'communication/networking/lrndis/lwip-1.4.1/src/netif/ethernetif.c',
    'communication/networking/lrndis/lwip-1.4.1/src/netif/etharp.c',
    'communication/networking/lrndis/lwip-1.4.1/src/core/timers.c',
    'communication/networking/lrndis/lwip-1.4.1/src/core/dns.c',
    'communication/networking/lrndis/lwip-1.4.1/src/core/ipv4/ip_addr.c',
    'communication/networking/lrndis/lwip-1.4.1/src/core/ipv4/igmp.c',
    'communication/networking/lrndis/lwip-1.4.1/src/core/ipv4/icmp.c',
    'communication/networking/lrndis/lwip-1.4.1/src/core/ipv4/inet_chksum.c',
    'communication/networking/lrndis/lwip-1.4.1/src/core/ipv4/ip_frag.c',
    'communication/networking/lrndis/lwip-1.4.1/src/core/ipv4/ip.c',
    'communication/networking/lrndis/lwip-1.4.1/src/core/ipv4/inet.c',
    'communication/networking/lrndis/lwip-1.4.1/src/core/ipv4/autoip.c',
    'communication/networking/lrndis/lwip-1.4.1/src/core/dhcp.c',
    'communication/networking/lrndis/lwip-1.4.1/src/core/tcp_in.c',
    'communication/networking/lrndis/lwip-1.4.1/src/core/init.c',
    'communication/networking/lrndis/lwip-1.4.1/src/core/def.c',
    'communication/networking/lrndis/lwip-1.4.1/src/core/tcp.c',
    'communication/networking/lrndis/lwip-1.4.1/src/core/snmp/mib2.c',
    'communication/networking/lrndis/lwip-1.4.1/src/core/snmp/asn1_dec.c',
    'communication/networking/lrndis/lwip-1.4.1/src/core/snmp/msg_in.c',
    'communication/networking/lrndis/lwip-1.4.1/src/core/snmp/msg_out.c',
    'communication/networking/lrndis/lwip-1.4.1/src/core/snmp/asn1_enc.c',
    'communication/networking/lrndis/lwip-1.4.1/src/core/snmp/mib_structs.c',
    'communication/networking/lrndis/lwip-1.4.1/src/core/raw.c',
    'communication/networking/lrndis/lwip-1.4.1/src/core/pbuf.c',
    'communication/networking/lrndis/lwip-1.4.1/src/core/stats.c',
    'communication/networking/lrndis/lwip-1.4.1/src/core/udp.c',
    'communication/networking/lrndis/lwip-1.4.1/src/core/netif.c',
    'communication/networking/lrndis/lwip-1.4.1/src/core/sys.c',
    'communication/networking/lrndis/lwip-1.4.1/src/core/mem.c',
    'communication/networking/lrndis/lwip-1.4.1/src/core/tcp_out.c',
    'communication/networking/lrndis/lwip-1.4.1/src/core/memp.c',
}

build{
    name='ODriveFirmware',
    toolchains={toolchain},
    --toolchains={LLVMToolchain('x86_64', {'-Ofast'}, {'-flto'})},
    packages={'stm_platform'},
    sources=array_concat({
        'Drivers/DRV8301/drv8301.c',
        'MotorControl/utils.c',
        'MotorControl/arm_sin_f32.c',
        'MotorControl/arm_cos_f32.c',
        'MotorControl/low_level.cpp',
        'MotorControl/nvm.c',
        'MotorControl/axis.cpp',
        'MotorControl/motor.cpp',
        'MotorControl/encoder.cpp',
        'MotorControl/controller.cpp',
        'MotorControl/sensorless_estimator.cpp',
        'MotorControl/trapTraj.cpp',
        'MotorControl/main.cpp',
        'communication/communication.cpp',
        'communication/ascii_protocol.cpp',
        'communication/interface_uart.cpp',
        'communication/interface_usb.cpp',
        'communication/interface_can.cpp',
        'communication/interface_i2c.cpp',
        'fibre/cpp/protocol.cpp',
        'FreeRTOS-openocd.c'
    }, network_sources),
    includes={
        'Drivers/DRV8301',
        'MotorControl',
        'fibre/cpp/include',
        'communication/networking/lrndis/lwip-1.4.1/src/include',
        'communication/networking/lrndis/lwip-1.4.1/src/include/ipv4',
        'communication/networking/lrndis/lwip-1.4.1/apps/httpserver_raw',
        '.'
    }
}
