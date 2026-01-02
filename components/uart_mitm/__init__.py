import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart
from esphome.const import CONF_ID

DEPENDENCIES = ['uart']

serial_ns = cg.esphome_ns.namespace('serial')

UARTMITM = serial_ns.class_('UARTMITM', cg.Component)

CONF_UART1 = "uart1"
CONF_UART2 = "uart2"
CONF_UART1_NAME = "uart1_name"
CONF_UART2_NAME = "uart2_name"
CONF_DEBUG = "debug"
CONF_FILTER = "filter"
CONF_INCLUDE = "include"
CONF_EXCLUDE = "exclude"
CONF_PACKET_HEADERS = "packet_headers"

DEBUG_SCHEMA = cv.Schema({
    cv.Optional(CONF_UART1_NAME, default="UART1"): cv.string,
    cv.Optional(CONF_UART2_NAME, default="UART2"): cv.string,
    cv.Optional(CONF_FILTER): cv.ensure_list(cv.ensure_list(cv.hex_uint8_t)),
    cv.Optional(CONF_INCLUDE): cv.ensure_list(cv.ensure_list(cv.hex_uint8_t)),
    cv.Optional(CONF_EXCLUDE): cv.ensure_list(cv.ensure_list(cv.hex_uint8_t)),
    cv.Optional(CONF_PACKET_HEADERS): cv.ensure_list(cv.ensure_list(cv.hex_uint8_t)),
})

CONFIG_SCHEMA = cv.COMPONENT_SCHEMA.extend({
    cv.GenerateID(): cv.declare_id(UARTMITM),
    cv.Required(CONF_UART1): cv.use_id(uart.UARTComponent),
    cv.Required(CONF_UART2): cv.use_id(uart.UARTComponent),
    cv.Optional(CONF_DEBUG): cv.Any(cv.boolean, DEBUG_SCHEMA),
})


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    uart1 = await cg.get_variable(config[CONF_UART1])
    cg.add(var.set_uart1(uart1))
    uart2 = await cg.get_variable(config[CONF_UART2])
    cg.add(var.set_uart2(uart2))
    
    if CONF_DEBUG in config:
        debug_conf = config[CONF_DEBUG]
        # If debug is just a boolean (true), skip detailed configuration
        if isinstance(debug_conf, bool):
            return
        if CONF_UART1_NAME in debug_conf:
            cg.add(var.set_uart1_name(debug_conf[CONF_UART1_NAME]))
        if CONF_UART2_NAME in debug_conf:
            cg.add(var.set_uart2_name(debug_conf[CONF_UART2_NAME]))
        if CONF_PACKET_HEADERS in debug_conf:
            for header in debug_conf[CONF_PACKET_HEADERS]:
                cg.add(var.add_packet_header(header))
        if CONF_FILTER in debug_conf:
            for message_pattern in debug_conf[CONF_FILTER]:
                cg.add(var.add_message_filter(message_pattern))
        if CONF_INCLUDE in debug_conf:
            for include_pattern in debug_conf[CONF_INCLUDE]:
                cg.add(var.add_include_filter(include_pattern))
        if CONF_EXCLUDE in debug_conf:
            for exclude_pattern in debug_conf[CONF_EXCLUDE]:
                cg.add(var.add_exclude_filter(exclude_pattern))
