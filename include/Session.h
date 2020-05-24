#pragma once

#include <functional>
#include <memory>
#include <vector>
#include <list>

#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/asio/deadline_timer.hpp>
#include "WorldPacket.h"
#include "Endian.h"

using boost::asio::deadline_timer;
namespace jt808sensor
{

namespace server
{

struct mediainfodescription
{
    uint8 mediatype;
    uint32 multimediaID;
    uint16 totalpacketNo;
    uint16 currentpacketNo;
};

class Session
{
public:
    using Socket = boost::asio::ip::tcp::socket;
    using TermCallback = std::function<void ()>;


    explicit Session(Socket&& sock)
        : m_socket(std::move(sock)),
          m_remote(m_socket.remote_endpoint()),
          heartbeat_timer_(m_socket.get_io_service()),
          mediadelay_timer_(m_socket.get_io_service()),
          _strand(m_socket.get_io_service()
          )
    {
        //heartbeat_timer_(m_socket.get_io_service());
        boost::asio::socket_base::receive_buffer_size recv_option(2*65536);
        m_socket.set_option(recv_option);
    };

    template <typename TFunc>
    void setTerminateCallback(TFunc&& func)
    {
        m_termCb = std::forward<TFunc>(func);
    }

    void start();
    /*
        using InputMsg =
            demo1::Message<
                comms::option::ReadIterator<const std::uint8_t*>,
                comms::option::Handler<Session>,
                comms::option::NameInterface
            >;

        using InSimpleInts = demo1::message::SimpleInts<InputMsg>;
        using InScaledInts = demo1::message::ScaledInts<InputMsg>;
        using InFloats = demo1::message::Floats<InputMsg>;
        using InEnums = demo1::message::Enums<InputMsg>;
        using InSets = demo1::message::Sets<InputMsg>;

        void handle(InBitfields& msg);
        void handle(InStrings& msg);
        void handle(InDatas& msg);
        void handle(InLists& msg);
        void handle(InOptionals& msg);
        void handle(InVariants& msg);
        void handle(InputMsg&);
        */
    void sendcheckpacket();
    void sendrestorepacket();
    void sendrealtimepacket();
    void sendreadconfigpacket();
    void sendactiveconfigpacket();
    void  sendactivesetconfigpacket();
    void sendadasconfigpacket();
    void sendchecksensorstatuspacket();

private:
void handle_writepacket(const boost::system::error_code& ec);
void sendtiimepacket(WorldPacket  packet);
void packetwrite(WorldPacket& m_packet);
void packetwrite();
void packetwrite_impl(WorldPacket& m_packet);
void writeHandler( const boost::system::error_code& error, const size_t bytesTransferred);

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
    void heart_tick(const boost::system::error_code& ec);

    void start_getalarmsmedia();
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
    void readmedia(std::size_t bytesCount);
	void sendcurrentmultimediarequest();

    void terminateSession();
    void processInput();
    //oid sendAck(demo1::MsgId id);

    Socket m_socket;
    boost::asio::io_service::strand _strand;
    std::size_t total_bytes_read;


    TermCallback m_termCb;
    boost::array<std::uint8_t, 1024*64> m_readBuf;
    std::unique_ptr<char[]> buf;
    unsigned int buf_size;
    std::vector<std::uint8_t> m_inputBuf;
    std::list<mediainfodescription> m_mediainfodescription;
	mediainfodescription t_mediainfodescription;//hold current media info request
	uint mediarequestcnt=0;
    uint8 m_mediatype;
    uint32 m_multimediaID;
    uint16 m_totalpacketNo;
    uint16 m_currentpacketNo;
    //Frame m_frame;
    Socket::endpoint_type m_remote;
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

using SessionPtr = std::unique_ptr<Session>;

} // namespace server

} // namespace demo1
