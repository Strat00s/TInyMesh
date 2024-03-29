/** @file tinyMesh.hpp
 * @author Lukáš Baštýř (l.bastyr@seznam.cz, 492875)
 * @brief TinyMesh is a simple protocol for IoT devices.
 * @version 0.1
 * @date 27-11-2023
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#ifdef ARDUINO
#include <Arduino.h>
#endif


/*# Packet structure
-------------HEADER-------------
    VERSION     8b
    SOURCE      8b
    DESTINATION 8b
    MESSAGE ID  16b
    FLAGS       8b
        7-6: REPEAT CNT
        5-2: MESSAGE TYPE
            0000 = OK
            0001 = ERR
            0010 = REGISTER
            0011 = PING
            0100 = STATUS
            0101 = TM_MSG_COMBINED
            0110 ... 1110 = RESERVED
            1111 = CUSTOM
        1-0: DEVICE TYPE
            00 = GATEWAY
            01 = NODE
            10 = LP_NODE
            11 = OTHER
    DATA LENGTH 8b
--------------DATA--------------
    DATA...     256b - len(header)

# Predefined messages packet structure and flow:
    OK
        VERSION
        SOURCE
        DESTINATION
        MESSAGE ID
        FLAGS
            7-6: REPEAT CNT
            5-2: MESSAGE TYPE: TM_MSG_OK
            1-0: DEVICE TYPE
        DATA LENGTH: l
        DATA: l bytes (depends on answered message)
    ERROR
        VERSION
        SOURCE
        DESTINATION
        MESSAGE ID
        FLAGS
            7-6: REPEAT CNT
            5-2: MESSAGE TYPE: TM_MSG_ERR
            1-0: DEVICE TYPE
        DATA LENGTH: 1
        DATA: ERROR_CODE
    PING
        VERSION
        SOURCE
        DESTINATION
        MESSAGE ID
        FLAGS
            7-6: REPEAT CNT
            5-2: MESSAGE TYPE: TM_MSG_PING
            1-0: DEVICE TYPE
        DATA LENGTH: 0
        DATA...
    REGISTER
        VERSION
        SOURCE
        DESTINATION: 255
        MESSAGE ID
        FLAGS
            7-6: REPEAT CNT
            5-2: MESSAGE TYPE: TM_MSG_REGISTER
            1-0: DEVICE TYPE
        DATA LENGTH: 0
        DATA...
    STATUS
        VERSION
        SOURCE
        DESTINATION
        MESSAGE ID
        FLAGS
            7-6: REPEAT CNT
            5-2: MESSAGE TYPE: TM_MSG_STATUS
            1-0: DEVICE TYPE
        DATA LENGTH: l
        DATA: string of size l
    Custom
        VERSION             1
        DEVICE TYPE         x
        MESSAGE ID          n
        SOURCE ADDRESS      y
        DESTINATION ADDRESS z
        PORT NUMBER         p
        MESSAGE TYPE        DATA
        DATA LENGTH         l
        DATA...             CUSTOM (l bytes)
*/
//TODO save only here
//TODO fix flags...


#define TM_VERSION 1

//Flag bit locations
#define TM_NODE_TYPE_LSB 0
#define TM_NODE_TYPE_MSB 1
#define TM_MSG_TYPE_LSB  2
#define TM_MSG_TYPE_MSB  5
#define TM_RPT_CNT_LSB   6
#define TM_RPT_CNT_MSB   7


//RETURN FLAGS
#define TM_OK                   0b00000000
#define TM_ERR_NULL             0b00000001 //packet is null or given buffer is null
#define TM_ERR_DATA_NULL        0b00000010
#define TM_ERR_VERSION          0b00000100
#define TM_ERR_ADDRESS          0b00001000
#define TM_ERR_DATA_LENGTH      0b00010000
#define TM_ERR_MSG_TYPE         0b00100000
#define TM_ERR_MSG_TYPE_LEN     0b01000000
#define TM_ERR_BUF_LEN          0b10000000

//TM_ERR_MESSAGES 
#define TM_ERR_MSG_UNHANDLED     1
#define TM_ERR_SERVICE_UNHANDLED 2
#define TM_ERR_ADDRESS_LIMIT     3

//Check packet returns
#define TM_PACKET_DUPLICATE    0b00000001
#define TM_PACKET_RESPONSE     0b00000010
#define TM_PACKET_RND_RESPONSE 0b00000100
#define TM_PACKET_REQUEST      0b00001000
#define TM_PACKET_FORWARD      0b00010000
#define TM_PACKET_REPEAT       0b00100000

#define TM_MAX_REPEAT 0b11

/*----(MESSAGE TYPES)----*/
#define TM_MSG_OK       0b0000 //response can't be brodcast
#define TM_MSG_ERR      0b0001 //response can't be brodcast
#define TM_MSG_REGISTER 0b0010 //register to gateway (and get address)
#define TM_MSG_PING     0b0011 //ping a node
#define TM_MSG_STATUS   0b0100 //send string status
#define TM_MSG_COMBINED 0b0101 //combine multiple packets into one
#define TM_MSG_CUSTOM   0b1111 //send custom data

/*----(NODE TYPES)----*/
#define TM_NODE_TYPE_GATEWAY 0b00
#define TM_NODE_TYPE_NODE    0b01
#define TM_NODE_TYPE_LP_NODE 0b10
#define TM_NODE_TYPE_OTHER   0b11


//Packet part sizes
#define TM_HEADER_LENGTH 7 //header length in bytes
#ifndef TM_DATA_LENGTH
    #define TM_DATA_LENGTH   256 - TM_HEADER_LENGTH
#endif

//Default config
#ifndef TM_DEFAULT_ADDRESS
    #define TM_DEFAULT_ADDRESS   0 //default address
#endif
#ifndef TM_BROADCAST_ADDRESS
    #define TM_BROADCAST_ADDRESS 255 //default broadcast address
#endif
#ifndef TM_SENT_QUEUE_SIZE
    #define TM_SENT_QUEUE_SIZE   10 //array of uint32
#endif
#ifndef TM_CLEAR_TIME
    #define TM_CLEAR_TIME 3000 //time before clearing entire sent queue in ms
#endif


#define ARRAY_CMP(a1, a2, len) (memcmp(a1, a2, len) == 0) //compare len of arrays against each other


typedef union{
    struct {
        uint8_t version;
        uint8_t source;
        uint8_t destination;
        uint8_t msg_id_msb;
        uint8_t msg_id_lsb;
        uint8_t flags;
        uint8_t data_length;
        uint8_t data[TM_DATA_LENGTH];
    } fields;
    uint8_t raw[TM_HEADER_LENGTH + TM_DATA_LENGTH];
} packet_t;

typedef union {
    struct {
        uint8_t source;
        uint8_t destination;
        uint8_t msg_id_msb;
        uint8_t msg_id_lsb;
        uint8_t repeat;
    } fields;
    uint8_t raw[5];
} packet_id_t;


class TinyMesh {
private:
    uint8_t version                        = TM_VERSION;           //supported TinyMesh version
    uint8_t address                        = TM_DEFAULT_ADDRESS;   //this NODE address
    uint8_t gateway                        = TM_BROADCAST_ADDRESS; //gateway address
    uint8_t node_type                      = TM_NODE_TYPE_NODE;   //this NODE type
    packet_id_t sent_queue[TM_SENT_QUEUE_SIZE] = {0};
    uint32_t last_msg_time                 = 0;

  #ifndef ARDUINO
    unsigned long (*millis)() = nullptr;
  #endif

    /** @brief Compare len items in array against a specific value*/
    template<typename T1, typename T2>
    bool arrayValCmp(T1 array, T2 val, size_t len) {
        for (size_t i = 0; i < len; i++) {
            if (array[i] != val)
                return false;
        }
        return true;
    }

    bool isDuplicate(packet_t *packet) {
        packet_id_t pid = createPacketID(packet);
        for (auto &spid : this->sent_queue) {
            if (ARRAY_CMP(spid.raw, pid.raw, 5))
                return true;
        }
        return false;
    }

public:

    /** @brief Create TinyMesh instance.
     * The class containes a simple LCG for pseudo random message ID generation where the seed is used.
     * @param node_type Node type
     * @param seed Starting seed for LCG
     */
    TinyMesh(uint8_t node_type);

    /** @brief Create TinyMesh instance.
     * The class containes a simple LCG for pseudo random message ID generation where the seed is used.
     * @param node_type Node type
     * @param address Node address
     * @param seed Starting seed for LCG
     */
    TinyMesh(uint8_t node_type, uint8_t address);

    ~TinyMesh();


  #ifndef ARDUINO
    /** @brief Register time keeping function for creating timestamps for packets.
     * It is expected that this function returns time in milliseconds between individual calls of this function.
     * If other time unit is used, edit TM_TIME_TO_STALE macro (or redefine it).
     * 
     * @param millis Function pointer to function returning milliseconds since boot
     */
    void registerMillis(unsigned long (*millis)());
  #endif

    void setSeed(uint16_t seed = 42069);


    /** @brief Set protocol version.
     * Max version is TM_VERSION.
     * @param version 
     */
    void setVersion(uint8_t version);

    /** @brief Set node address.
     * Address 255 will be converted to 0
     * 
     * @param address 
     */
    void setAddress(uint8_t address);

    /** @brief Set gateway address after registration
     * 
     * @param address Gateway address
     */
    void setGatewayAddress(uint8_t address);
    
    /** @brief Set node type.
     * Unknown node types will be converted to TM_TYPE_OTHER.
     * 
     * @param node_type
     */
    void setNodeType(uint8_t node_type);

    /** @brief Set message ID for specified packet.
     * 
     * @param packet Packet for which to set message ID
     * @param message_id New message ID
     */
    void setMessageId(packet_t *packet, uint16_t message_id);


    uint8_t getVersion();
    uint8_t getAddress();
    uint8_t getGatewayAddress();
    uint8_t getNodeType();

    /** @brief Construct and return the message ID.
     * 
     * @param packet Packet from which to get message ID
     * @return Message ID
     */
    uint16_t getMessageId(packet_t *packet);


    /** @brief Build packet from specific data.
     * Runs checkHeader() at the end.
     * Saves packet if valid.
     * 
     * @param packet Packet poiner where to store header and data
     * @param destination Destination node address
     * @param message_id Id of this message (can be tm.lcg())
     * @param message_type Message type
     * @param data Data which to send
     * @param length Length of data
     * @return TM_OK on succes, TM_ERR_... macros on error
     */
    uint8_t buildPacket(packet_t *packet, uint8_t destination, uint16_t message_id, uint8_t message_type = TM_MSG_CUSTOM, uint8_t *data = nullptr, uint8_t length = 0, uint8_t repeat_cnt = 0);

    /** @brief Build packet from raw data.
     * Check packet if it is valid (checkHeader).
     * Saves packet if valid.
     * 
     * @param packet Packet poiner where to store header and data
     * @param buffer Buffer with raw packet
     * @param length Buffer length
     * @return TM_OK on succes, TM_ERR_... macros on error
     */
    uint8_t buildPacket(packet_t *packet, uint8_t *buffer, uint8_t length);

    /** @brief Check if stored packet has valid header and header data.
     * 
     * @param packet Packet to check
     * @return TM_OK on succes, TM_ERR_... macros on error
     */
    uint8_t checkHeader(packet_t *packet);


    /** @brief Check packets relation to this node.
     * Used for received packets.
     * Saves packet if valid.
     * 
     * @param packet Packet to check
     * @return TM_OK on succes, TM_ERR_... macros on error
     */
    uint8_t checkPacket(packet_t *packet);

    /** @brief Save packet id directly to sent queue
     * 
     * @param packet_id Packet id to save
     */
    void savePacketID(packet_id_t packet_id);

    /** @brief Save packet ID from packet to sent queue for duplicity checks
     * 
     * @param packet Recently transmitted/received packet
     */
    void savePacket(packet_t *packet);

    /** @brief Clear queue of received/transmitted messages.*/
    uint8_t clearSentQueue(bool force = false);

    packet_id_t createPacketID(packet_t *packet);
    packet_id_t createPacketID(uint8_t source, uint8_t destination, uint16_t message_id, uint8_t repeat_cnt);


    uint16_t lcg(uint16_t seed = 0);

    //flag management helpers

    /** @brief Set specific bits in x to val
     * 
     * @tparam T1 
     * @tparam T2 
     * @param x 
     * @param val 
     * @param msb 
     * @param lsb 
     */
    template<typename T1, typename T2>
    void setBits(T1 *x, T2 val, uint8_t msb, uint8_t lsb) {
        T1 mask = ((T1)1 << (msb - lsb + 1)) - 1;
        mask <<= lsb;
        *x = (*x & ~mask) | ((val << lsb) & mask);
    }

    /** @brief Get specific bits from x shifted to start from 1st (lsb) bit*/
    template<typename T>
    T getBits(T x, uint8_t msb, uint8_t lsb) {
        return (x >> lsb) & (((T)1 << (msb - lsb + 1)) - 1);
    }
};
