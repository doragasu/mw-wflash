/************************************************************************//**
 * \file
 *
 * \brief Memory Pool.
 *
 * Implements a memory pool for dynamic memory allocation. This memory pool
 * gets memory from the unused region between the end of the .bss section
 * and the stack top. The implementation is pretty simple: an internal
 * pointer grows when memory is requested, and is reset to the specified
 * position to free memory. This restricts the usage of the module to
 * scenarios that free memory in exactly the reverse order in which they
 * requested it (it does not allow generic allocate/free such as malloc()
 * does).
 *
 * \author doragasu
 * \date   2017
 ****************************************************************************/

#include <string.h>

#include "mpool.h"

#define MP_ALIGN_MASK	(MP_ALIGN - 1)

/// BSS end symbol, defined in linker script. Pool grows from here to the
/// end of the RAM
extern uint8_t _eflash;

/// End of the memory POOL
#define MP_POOL_END		((void*)0x01000000)

/// Mask used for alignment computations
#define MP_ALIGN_MASK 	(MP_ALIGN - 1)

#define MP_ALIGN_COMP(addr)	(uint8_t*)(((((uint32_t)(addr)) + MP_ALIGN_MASK) \
			& (~((uint32_t)MP_ALIGN_MASK))))

typedef struct {
	uint8_t *floor;
	uint8_t *pos;
} MpData;

/// Local module data
MpData md;

/************************************************************************//**
 * \brief Pool initialization. 
 *
 * Call this function before using any other in this module
 ****************************************************************************/
void MpInit(void) {
	// Ensure the origin is aligned and initialize current position
	md.floor = MP_ALIGN_COMP(&_eflash);
	md.pos = md.floor;
}

/************************************************************************//**
 * \brief Allocates data from the pool.
 *
 * Obtains a memory chunck with enforced alignment and requested length.
 * Obtained chunck is contiguous.
 *
 * \param[in] length Length of the contiguous section to allocate.
 *
 * \return Pointer to the allocated memory zone of the requested length, or
 * NULL if the allocation could not succeed.
 *
 * \todo Maybe allocation should be tested against the stack pointer
 ****************************************************************************/
void *MpAlloc(uint16_t length) {
	void *tmp, *ret;

	// Adjust length depending on alignmentnforcement
	length = (length + MP_ALIGN_MASK) & ~MP_ALIGN_MASK;

	// Check there is enough room
	tmp = length + md.pos;
	if (tmp >= MP_POOL_END) return NULL;
	ret = md.pos;
	md.pos = tmp;

	return ret;
}

/************************************************************************//**
 * \brief Free memory up to the one pointed by pos.
 *
 * Memory previously allocated is freed up to pos position. The usual (and
 * recommended) way of using this function is calling it with pos set to the
 * value of the last MpAlloc() call.
 *
 * \warning This function will free memory requested by several MpAlloc()
 * calls if the input pos pointer is not the last returned by MpAlloc(). E.g.
 * if MpAlloc() is called consecutively three times, and MpFreeTo() is called
 * with the pointer returned by the second call, the memory granted by the
 * second and third calls to MpAlloc() will be deallocated. Although this is
 * usually undesired, sometimes it is useful. In any case it is important
 * taking it into account when using this module.
 *
 * \warning Function fails (and does nothing) if input position is NOT
 * aligned as required by MP_ALIGN parameter. As MpAlloc() always returns
 * aligned pointers, this should not be a problem when using the function as
 * intended (passing pointers obtained by MpAlloc() calls).
 ****************************************************************************/
void MpFreeTo(void *pos) {
	// Check pos looks valid, and set it if affirmative
	if ((pos >= (void*)md.floor) && (pos < (void*)md.pos) &&
			(pos == MP_ALIGN_COMP(pos))) md.pos = pos;
}

