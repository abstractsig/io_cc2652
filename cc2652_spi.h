/*
 *
 * cc2652 synchronous serial interface implementing the SPI protocol
 *
 */
#ifndef cc2652_spi_H_
#define cc2652_spi_H_
#include <io_cpu.h>
#include <memory/io_spi_flash.h>



typedef struct PACK_STRUCTURE cc2652_spi_direct {
	IO_SPI_SOCKET_STRUCT_MEMBERS

	uint32_t base_address;

} cc2652_spi_direct_t;

extern EVENT_DATA io_socket_implementation_t cc2652_direct_spi_implementation;


#ifdef IMPLEMENT_IO_CPU
//-----------------------------------------------------------------------------
//
// Implementation
//
//-----------------------------------------------------------------------------

static io_socket_t*
cc2652_spi_initialise (io_socket_t *socket,io_t *io,io_settings_t const *C) {
	initialise_io_socket (socket,io);
	return socket;
}

EVENT_DATA io_socket_implementation_t cc2652_direct_spi_implementation = {
	SPECIALISE_IO_SOCKET_IMPLEMENTATION (
		&io_spi_socket_implementation
	)
	.initialise = cc2652_spi_initialise,
};

#endif /* IMPLEMENT_IO_CPU */
#endif
/*
Copyright 2020 Gregor Bruce

Permission to use, copy, modify, and/or distribute this software for any purpose
with or without fee is hereby granted, provided that the above copyright notice
and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT,
OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/
