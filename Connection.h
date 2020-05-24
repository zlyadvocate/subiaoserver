#pragma once

#include <stdint.h>
#include <vector>
#include <functional>
#include <memory>
#include <list>

#include <asio.hpp>
#include <boost/array.hpp>
#include <boost/asio/deadline_timer.hpp>
#include "WorldPacket.h"
#include "Endian.h"




using namespace boost::asio;
class ConnectionManager;
struct mediainfodescription
{
    uint8 mediatype;
    uint32 multimediaID;
    uint16 totalpacketNo;
    uint16 currentpacketNo;
};


class Connection : public std::enable_shared_from_this<Connection>
{
public:
    explicit Connection(io_service& io_service, ConnectionManager& manager
                       );
    ip::tcp::socket& socket();
    void Start();
    void start();
    void Stop();
    void packetwrite(WorldPacket& m_packet);


    std::shared_ptr<Connection> getptr()
    {
        return shared_from_this();
    }
private:


    boost::asio::io_service::strand _strand;
    std::size_t total_bytes_read;

    uint16_t maxKeySize_;
    boost::asio::ip::tcp::socket socket_;
    ConnectionManager& connectionManager_;

    std::shared_ptr<std::vector<uint8_t>> data_;
    void heart_tick(const boost::system::error_code& ec);
    void readmedia(std::size_t bytesCount);

    void sendcheckpacket();
    void sendrestorepacket();
    void sendrealtimepacket();
    void sendreadconfigpacket();
    void sendactiveconfigpacket();
    void sendactivequerypacket();
    void sendactivesetconfigpacket();
    void sendADASactivesetconfigpacket();
    void sendDSMactivesetconfigpacket();
    void sendadasconfigpacket();
    void sendchecksensorstatuspacket();

private:
    void handle_writepacket(const boost::system::error_code& ec);
    void sendtiimepacket(WorldPacket  packet);

    void packetwrite();
    void packetwrite_impl(WorldPacket& m_packet);
    void writeHandler( const boost::system::error_code& error, const size_t bytesTransferred);
    //0x2F ack
    void handlecheckpacket_ack();
//0x30 ack
    void handlerestorepacket_ack();
//0x32 ack
    void handlereadconfigpacket_ack();
//0x34 ack
    void handleactivesetconfigpacket_ack();

//0x35
    void handleactivesetconfigpacket();
    void  handleADASactivesetconfigpacket();
    void  handleDSMactivesetconfigpacket();


//op code =38
    void handlesensorstatus();
//opcaode 50
    void sendmultimediarequest();

    void handlattachmentload(WorldPacket &);
//0x51 response
    void sendattachmentload_ack();
//0x52 photo now!
    void sendphotenowpacket();
//0x52_ack
    void handlphotonowack();

//heartbeattimer
    void start_write();
    void handle_write(const boost::system::error_code& ec);


    void start_getalarmsmedia(int mediatype);
    void getalarmsmedia(const boost::system::error_code& ec );
//0x38
    void handle(WorldPacket &);
    void handlestatusreport(WorldPacket &);
    void handlealarmreport(WorldPacket &);
    void handlelarmdata(WorldPacket &);
    void handladasealarmreport(WorldPacket &);
    void handledsmalarmreport(WorldPacket &);
    void sendpacket(WorldPacket  packet);
//0x51
    void handlattachmentload();
    void sendmultimediaack();//0x51 ack

    void sendcurrentmultimediarequest();

    void terminateSession();
    void processInput();
    //oid sendAck(demo1::MsgId id);


    boost::array<std::uint8_t, 1024*64> m_readBuf;
    std::unique_ptr<char[]> buf;
    unsigned int buf_size;
    std::vector<std::uint8_t> m_inputBuf;
    std::list<mediainfodescription> m_mediainfodescription;
    mediainfodescription t_mediainfodescription;//hold current media  request info
    uint mediarequestcnt=0;
    uint8 m_mediatype;
    uint32 m_multimediaID;
    uint16 m_totalpacketNo;
    uint16 m_currentpacketNo;

    uint8 s_jt808sync;
    uint8 s_checksum;
    uint8 s_terminalcodeID;
    uint16 s_manufactureID;
    uint16 s_sequencemsgcnt;

    uint8 s_opcode;
    deadline_timer heartbeat_timer_;
    deadline_timer mediadelay_timer_;
    uint8 mediafile_checksum;
    //s_jt808sync>>s_checksum>>s_sequencemsgcnt>>s_manufactureID>>s_terminalcodeID>>s_opcode;
    WorldPacket readpacket;
    WorldPacket mediapacket;
    typedef std::deque<WorldPacket> Outbox;
    Outbox _outbox;
    size_t headtailpos;
    bool dataok;
    FILE* m_file=NULL;
};
using ConnectionPtr = std::unique_ptr<Connection>;
