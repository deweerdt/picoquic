/*
* Author: Christian Huitema
* Copyright (c) 2017, Private Octopus, Inc.
* All rights reserved.
*
* Permission to use, copy, modify, and distribute this software for any
* purpose with or without fee is hereby granted, provided that the above
* copyright notice and this permission notice appear in all copies.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL Private Octopus, Inc. BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "../picoquic/picoquic_internal.h"

/*
 * Testing Arrival of Frame for Stream Zero
 *
 * Stream first byte is formatted as 11FSSOOD. These bits are parsed as follows:
 *
 * - The first two bits must be set to 11, indicating that this is a STREAM frame.
 *
 * - F is the FIN bit, which is used for stream termination.
 *
 * - The SS bits encode the length of the Stream ID header field. The values 00, 01, 
 *   02, and 03 indicate lengths of 8, 16, 24, and 32 bits long respectively.
 *
 * - The OO bits encode the length of the Offset header field. The values 00, 01, 02, 
 *   and 03 indicate lengths of 0, 16, 32, and 64 bits long respectively.
 *
 * 
 * A STREAM frame is shown below.
 *
 *  0                   1                   2                   3
 * 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *| Stream ID(8 / 16 / 24 / 32)                   ...
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *| Offset(0 / 16 / 32 / 64)                    ...
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *| [Data Length(16)] | Stream Data(*)      ...
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 */
static uint8_t f0_1[] = {
    0xC0, /* Start Byte: F=0, SS=0, OO=0, D=0 */
    0, /* One byte stream ID */
    1 , 2, 3, 4, 5, 6, 7, 8, 9, 10 /* Some random data */
};

static uint8_t f0_2[] = {
    0xC2, /* Start Byte: F=0, SS=0, OO=1, D=0 */
    0, /* One byte stream ID */
    0, 10, /* Two byte offset */
    11 , 12, 13, 14, 15, 16, 17, 18, 19, 20 /* Some random data */
};

static uint8_t f0_3[] = {
    0xCA, /* Start Byte: F=0, SS=1, OO=1, D=0 */
    0, 0, /* Two  byte stream ID */
    0, 20, /* Two byte offset */
    21 , 22, 23, 24, 25, 26, 27, 28, 29, 30 /* Some random data */
};

static uint8_t f0_4[] = {
    0xD5, /* Start Byte: F=0, SS=2, OO=2, D=1 */
    0, 0, 0, /* Three  byte stream ID */
    0, 0, 0, 30, /* Four byte offset */
    0, 10, /* two byte length */
    31 , 32, 33, 34, 35, 36, 37, 38, 39, 40 /* Some random data */
};

static uint8_t f0_5[] = {
    0xDF, /* Start Byte: F=0, SS=3, OO=3, D=1 */
    0, 0, 0, 0, /* Four  byte stream ID */
    0, 0, 0, 0, 0, 0, 0, 40, /* Eight byte offset */
    0, 10, /* Two byte length */
    41 , 42, 43, 44, 45, 46, 47, 48, 49, 50, /* Some random data */
    0, 0, 0, 0, 0 /* Some random padding */
};

static uint8_t f0_45_overlap[] = {
    0xC3, /* Start Byte: F=0, SS=0, OO=1, D=1 */
    0, /* Two  byte stream ID */
    0, 35, /* Two byte offset */
    0, 10, /* Two byte length */
    0xFF , 0xFF ,0xFF , 0xFF , 0xFF , 0xFF ,0xFF ,0xFF, 0xFF, 0xFF /* Some random data */
};

struct packet
{
    uint8_t * packet;
    size_t packet_length;
    size_t offset;
    size_t data_length;
    size_t invalid_length;
};

static struct packet list1[] = {
    { f0_1, sizeof(f0_1), 0, 10, 0},
    { f0_2, sizeof(f0_2), 10, 10, 0 },
    { f0_3, sizeof(f0_3), 20, 10, 0 },
    { f0_4, sizeof(f0_4), 30, 10, 0 },
    { f0_5, sizeof(f0_5), 40, 10, 0 }
};


static struct packet list2[] = {
    { f0_2, sizeof(f0_2), 10, 10, 0 },
    { f0_3, sizeof(f0_3), 20, 10, 0 },
    { f0_1, sizeof(f0_1), 0, 10, 0 },
    { f0_5, sizeof(f0_5), 40, 10, 0 },
    { f0_4, sizeof(f0_4), 30, 10, 0 },
};

static struct packet list3[] = {
    { f0_1, sizeof(f0_1), 0, 10, 0 },
    { f0_2, sizeof(f0_2), 10, 10, 0 },
    { f0_3, sizeof(f0_3), 20, 10, 0 },
    { f0_2, sizeof(f0_2), 10, 10, 0 },
    { f0_3, sizeof(f0_3), 20, 10, 0 },
    { f0_4, sizeof(f0_4), 30, 10, 0 },
    { f0_4, sizeof(f0_4), 30, 10, 0 },
    { f0_5, sizeof(f0_5), 40, 10, 0 },
    { f0_45_overlap, sizeof(f0_45_overlap), 35, 10, 0 }
};

struct test_case_st
{
    struct packet * list;
    size_t list_size;
    size_t expected_length;
};

static struct test_case_st test_case[] = {
    { list1, sizeof(list1) / sizeof(struct packet), 50},
    { list2, sizeof(list2) / sizeof(struct packet), 50 },
    { list3, sizeof(list3) / sizeof(struct packet), 50 }
};

size_t nb_test_cases = sizeof(test_case) / sizeof(struct test_case_st);

static int StreamZeroFrameOneTest(struct test_case_st * test)
{
    int ret = 0;
    picoquic_cnx_t cnx = { 0 };
    size_t consumed = 0;
    uint64_t current_time = 0;

    for (size_t i = 0; ret == 0 && i < test->list_size; i++)
    {
        if (0 != picoquic_decode_stream_frame(&cnx, test->list[i].packet,
            test->list[i].packet_length, 1, &consumed, current_time))
        {
            ret = -1;
        }
    }

    if (ret == 0)
    {
        /* Check the content of all the data in the context */
        picoquic_stream_data * data = cnx.first_stream.stream_data;
        size_t data_rank = 0;

        while (data != NULL)
        {
            if (data->bytes == NULL)
            {
                ret = -1;
            }

            for (size_t i = 0; ret == 0 && i < data->length; i++) {
                data_rank++;
                if (data->bytes[i] != data_rank)
                {
                    ret = -1;
                }
            }

            data = data->next_stream_data;
        }

        if (ret == 0 && data_rank != test->expected_length)
        {
            ret = -1;
        }
    }

    return ret;
}

int StreamZeroFrameTest()
{
    int ret = 0;

    for (size_t i = 0; i < nb_test_cases; i++)
    {
        ret = StreamZeroFrameOneTest(&test_case[i]);
    }

    return ret;
}
