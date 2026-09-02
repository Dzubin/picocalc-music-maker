# Serial

The serial port is available throught the USB C port at the top of the PicoCalc or through the header on the left-side.

## serial_init

`void serial_init(uint baudrate, uint databits, uint stopbits, uart_parity_t parity)`

Initial the serial port.

### Parameters

- baudrate – Baudrate of UART in Hz
- databits – Number of bits of data, 5 through 8
- stopbits – Number of stop bits, 1 or 2
- parity – one of UART_PARITY_NONE, UART_PARITY_EVEN, UART_PARITY_ODD


## serial_char_available

`bool serial_input_available(void)`

Returns true if a serial character is available.


## serial_get_char

`char serial_get_char(void)`

Returns the next character from the serial input buffer; blocks if input is not available.


## serial_output_available

`bool serial_output_available(void)`

Returns true is a character can be emitted.


## serial_put_char

`void serial_put_char(char ch)`

Emits a character through the serial port.

### Parameters

- ch - the character to emit


