/* Host-test bridge to the production CAN transport and wire codec. */
#include "fw_update_protocol.h"
#include <string.h>

static FirmwareUpdateCanTransport peer;

void PeerInit(FirmwareUpdateCanSendFunction send, uint8_t node)
{
    FirmwareUpdateCanInit(&peer, node, FW_UPDATE_CAN_TESTER_ADDRESS,
                     send, NULL, 250U);
}

int PeerInput(uint32_t id, const uint8_t *data, uint8_t dlc, uint32_t now)
{
    return FirmwareUpdateCanOnFrame(&peer, id, data, dlc, now);
}

void PeerPoll(uint32_t now)
{
    FirmwareUpdateCanPoll(&peer, now);
}

int PeerTake(uint8_t *output)
{
    const uint8_t *encoded;
    size_t length;
    FirmwareUpdateMessage message;
    if (!FirmwareUpdateCanPeekFrame(&peer, &encoded, &length)) {
        return 0;
    }
    /* Production C decoding must accept every Python-generated request. */
    int result = FirmwareUpdateFrameDecode(encoded, length, &message)
        ? (int)length : -1;
    memcpy(output, encoded, length);
    FirmwareUpdateCanConsumeFrame(&peer);
    return result;
}

int PeerReply(uint8_t type, uint32_t session, uint32_t sequence,
              const uint8_t *payload, uint16_t length, uint32_t now)
{
    uint8_t encoded[64];
    size_t size = FirmwareUpdateFrameEncode(type, session, sequence, payload,
                                      length, encoded, sizeof(encoded));
    return size && FirmwareUpdateCanSendFrame(&peer, encoded, size, now);
}

/**
 * @brief Exercises the production board's transmit capacity with raw encoded data.
 */
int PeerSendFrame(const uint8_t *encoded, uint16_t length, uint32_t now)
{
    return FirmwareUpdateCanSendFrame(&peer, encoded, length, now);
}
