/*
 * Copyright (C) 2014-2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* Trusty Random Number Generation library.
 * Provides a CSPRNG and an interface to a HWRNG if present. The functions
 * in this library are currently not threadsafe. It is designed to be used
 * from a single-threaded Trusty application.
 */
#include <openssl/aes.h>

#include <err.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <lib/rng/trusty_rng.h>
#include <interface/hwrng/hwrng.h>
#include <trusty_std.h>

/*
 *	This is the generic part of the trusty app rng service.
 *	A server implementation for retrieving hardware backed
 *	random numbers, used by trusty_rng_hw_rand,
 *	is required to be provided by a hardware
 *	specific backend at tipc port HWRNG_PORT.
 *
 *	Clients of this library are encouraged to use the
 *	trusty_rng_secure_rand rather than the trusty_rng_hw_rand
 *	routine, as the latter incurs an IPC penalty with connection
 *	overhead.
 */

#define AES256_KEY_SIZE		32

/* CSPRNG state */
static uint8_t entropy_pool[AES256_KEY_SIZE];
static uint64_t counter;
static uint8_t rand_bytes_buf[AES_BLOCK_SIZE];
static uint32_t rand_bytes_pos;
static bool rng_initialized;

static int aes_encrypt_ecb(uint8_t *dst, const uint8_t *src,
			const uint8_t *key_bytes, size_t key_size_bits)
{
	AES_KEY key;
	if (AES_set_encrypt_key(key_bytes, key_size_bits, &key) != 0)
		return ERR_INVALID_ARGS;

	AES_encrypt(src, dst, &key);
	return NO_ERROR;
}

static int trusty_rng_init(void)
{
	int err = NO_ERROR;

	if (rng_initialized)
		goto done;

	err = trusty_rng_hw_rand(entropy_pool, sizeof(entropy_pool));
	if (err)
		goto done;

	counter = 0;
	rand_bytes_pos = sizeof(rand_bytes_buf);
	rng_initialized = true;
done:
	return err;
}

int trusty_rng_secure_rand(uint8_t *data, size_t len)
{
	int err = NO_ERROR;

	if (!data || !len)
		return ERR_INVALID_ARGS;

	if (!rng_initialized) {
		err = trusty_rng_init();
		if (err)
			goto done;
	}

	while (len > 0) {
		if (rand_bytes_pos == sizeof(rand_bytes_buf)) {
			/* Increment the counter and put the counter value into a buffer */
			uint8_t counter_buf[AES_BLOCK_SIZE];
			++counter;
			memset(counter_buf, 0, sizeof(counter_buf));
			memcpy(counter_buf, &counter, sizeof(counter));

			/* Encrypt the counter, placing the results in rand_bytes_buf */
			err = aes_encrypt_ecb(rand_bytes_buf, counter_buf, entropy_pool,
					sizeof(entropy_pool) * 8);
			if (err)
				goto done;

			rand_bytes_pos = 0;
		}

		/* Copy random bytes into the output buffer */
		while (rand_bytes_pos < sizeof(rand_bytes_buf) && len > 0) {
			*data++ = rand_bytes_buf[rand_bytes_pos++];
			len--;
		}
	}
done:
	return err;
}

int trusty_rng_add_entropy(const uint8_t *data, size_t len)
{
	/* Re-initialize the entropy pool, seeding from CSPRNG (based on current pool) */
	uint8_t new_pool[sizeof(entropy_pool)];
	u_int i;
	int err;

	err = trusty_rng_secure_rand(new_pool, sizeof(new_pool));
	if (err)
		goto done;

	memcpy(entropy_pool, new_pool, sizeof(new_pool));

	/* XOR new entropy bytes into the pool */
	for (i = 0; (i < sizeof(entropy_pool)) && (i < len); ++i)
		entropy_pool[i] ^= data[i];
done:
	return err;
}

__WEAK int trusty_rng_hw_rand(uint8_t *data, size_t len)
{
	struct hwrng_req req_hdr = {
		.len = len
	};

	iovec_t tx_iov = {
		.base = &req_hdr,
		.len = sizeof(req_hdr),
	};

	ipc_msg_t tx_msg = {
		.iov = &tx_iov,
		.num_iov = 1,
	};

	iovec_t rx_iov = {
		.base = data,
		.len = len,
	};
	ipc_msg_t rx_msg = {
		.iov = &rx_iov,
		.num_iov = 1,
	};

	long rc = connect(HWRNG_PORT, IPC_CONNECT_WAIT_FOR_PORT);
	if (rc < 0) {
		return rc;
	}

	handle_t chan = (handle_t) rc;

	rc = send_msg(chan, &tx_msg);
	if (rc < 0) {
		goto err;
	}

	if (rc != sizeof(req_hdr)) {
		rc = ERR_IO;
		goto err;
	}

	while (rx_msg.iov[0].len > 0) {
		uevent_t uevt;
		rc = wait(chan, &uevt, -1);
		if (rc != NO_ERROR) {
			goto err;
		}

		ipc_msg_info_t inf;
		rc = get_msg(chan, &inf);
		if (rc != NO_ERROR) {
			goto err;
		}

		if (inf.len > rx_msg.iov[0].len) {
			// received too much data
			rc = ERR_BAD_LEN;
			goto err;
		}

		rc = read_msg(chan, inf.id, 0, &rx_msg);
		if (rc < 0) {
			goto err;
		}

		size_t rx_size = (size_t) rc;
		rx_msg.iov[0].base += rx_size;
		rx_msg.iov[0].len -= rx_size;
		put_msg(chan, inf.id);
	}

	rc = NO_ERROR;
err:
	close(chan);
	return rc;
}

