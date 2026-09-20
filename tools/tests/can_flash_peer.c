/* Host-test bridge to the production CAN transport and wire codec. */
#include "sunlite_ota_can_transport.h"
#include <string.h>

static SunliteOtaCanTransport peer;

void PeerInit(SunliteOtaCanSendFunction send, uint8_t node)
{
    SunliteOtaCanInit(&peer, node, SUNLITE_OTA_CAN_TESTER_ADDRESS,
                     send, NULL, 250U);
}

int PeerInput(uint32_t id, const uint8_t *data, uint8_t dlc, uint32_t now)
{
    return SunliteOtaCanOnFrame(&peer, id, data, dlc, now);
}

void PeerPoll(uint32_t now)
{
    SunliteOtaCanPoll(&peer, now);
}

int PeerTake(uint8_t *output)
{
    const uint8_t *encoded;
    size_t length;
    uint8_t raw[SUNLITE_OTA_MAX_RAW_FRAME];
    SunliteOtaMessage message;
    if (!SunliteOtaCanPeekFrame(&peer, &encoded, &length)) {
        return 0;
    }
    /* Production C decoding must accept every Python-generated request. */
    int result = SunliteOtaFrameDecode(encoded, length, raw, sizeof(raw), &message)
        ? (int)length : -1;
    memcpy(output, encoded, length);
    SunliteOtaCanConsumeFrame(&peer);
    return result;
}

int PeerReply(uint8_t type, uint32_t session, uint32_t sequence,
              const uint8_t *payload, uint16_t length, uint32_t now)
{
    uint8_t encoded[64];
    size_t size = SunliteOtaFrameEncode(type, session, sequence, payload,
                                      length, encoded, sizeof(encoded));
    return size && SunliteOtaCanSendFrame(&peer, encoded, size, now);
}
