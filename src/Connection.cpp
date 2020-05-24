//#include "stdafx.h"
#include "Connection.h"
#include <iostream>
 #include <sys/timeb.h>
#include <boost/asio/io_service.hpp>
#include <boost/bind.hpp>
#include "ConnectionManager.h"
using namespace boost::asio;
using boost::asio::deadline_timer;
enum terminalcode
{
    ADASID=0x64,
    DMSID=0x65,
    TPMSID=0x66,
    BODIDD=0x67
};

template <typename T>
T swap_endian(T u)
{
    static_assert (CHAR_BIT == 8, "CHAR_BIT != 8");

    union
    {
        T u;
        unsigned char u8[sizeof(T)];
    } source, dest;

    source.u = u;

    for (size_t k = 0; k < sizeof(T); k++)
        dest.u8[k] = source.u8[sizeof(T) - k - 1];

    return dest.u;
}


uint8 jt808sync=0x7e;
uint8 terminalcodeID=ADASID;
uint16 manufactureID=swap_endian<uint16_t>(0x1234);
uint16 sequencemsgcnt=swap_endian<uint32_t>(0x0000);

//0x34 drivers monitor
//speed and volumn
uint8 speedthreshold=30;//default is 30
uint8 volumn=6;//default
//************ active photo********************/
//defailt 0x00---don't start active photo
//0x01 constant time slice photoing
//0x02 contsant distance photoing
//0x03 photoing on card insert event
//0x04 reserved
//0xff no change
uint8 activephoestagy=00;//default 0x00---don't start active photo

//default is 3600seconds
//0 no photoing
//0xffff no change
//activate when activephoestagy is 0x01
uint16 timedslice=5;//default is 200

//default is 200meters
//0 no photoing
//0xffff no change
//activate when activephoestagy is 0x02
uint16 distanceslice=2;

uint8  picturequality=3;//1-10
uint8  pictureslice=2;//1-5 default is 2
//01-cif;02-halfd1,03-fulld1,04-vga,05-720p,06-1080p,0xFF no change
uint8 picresolution=0x01;
uint8 videoresolution=0x01;
uint8 reschach[10];
//21
uint16 smokingtime=180;//0-3600 default is 180,0xFFFF no change
uint16 calltime=120;//0-3600 default is 120,0xFFFF no change
//drown
uint8  drownvideotime=5;//0-60 default is 5,0 no record ;0xff no change
uint8 drownpicsno=0x3;//0-10,default is 3
uint8 drownpicslice=0x2;//
uint8 reschar=0x00;
//call
uint8 callvideotime=5;//0-60 default is 5,0 no record ;0xff no change
uint8 callpicsno=0x3;//0-10,default is 3
uint8 callpicslice=2;
//smoking
uint8 smokingvideotime=5;
uint8 smokingpicsno=3;
uint8 smokingpicslice=2;
//distraction
uint8 distractionvideotime=5;
uint8 distractionpicsno=3;
uint8 distractionpicslice=3;
//driver abnormal
uint8 abnormalvideotime=5;
uint8 abnormalpicsno=3;
uint8 abnormalpicslice=2;
//others
uint8  dsmkeepit[2];

//adas
//1-10 same as DSM
uint8 resevedadas[9];
uint8 obstacledistance=30;//10-50 default is 30 0xff no change
uint8 obstaclevideotime=5;//0-60 default is 5,0 no record ;0xff no change
uint8 obstaclepicsno=3;
uint8 obstaclepicslice=2;
//lane
uint8 departuretime=60;//
uint8 departurecnt=5;//
uint8 departurevideotime=5;//0-60 default is 5,0 no record ;0xff no change
uint8 departurepicsno=3;
uint8 departurepicslice=2;
//ldw
uint8 ldwvideotime=5;//0-60 default is 5,0 no record ;0xff no change
uint8 ldwpicsno=3;
uint8 ldwpicslice=2;
//fcw
uint8 fcwtime=27;//10-50,
uint8 fcwvideotime=5;//0-60 default is 5,0 no record ;0xff no change
uint8 fcwpicsno=3;
uint8 fcwpicslice=2;
//pcw
uint8 pcwtime=30;//10-50,
uint8 pcwvideotime=5;//0-60 default is 5,0 no record ;0xff no change
uint8 pcwpicsno=3;
uint8 pcwpicslice=2;
//carclose
uint8 carclosetime=30;//10-50,
uint8 carclosevideotime=5;//0-60 default is 5,0 no record ;0xff no change
uint8 carclosepicsno=3;
uint8 carclosepicslice=2;
//tsr
uint8 tsrpicsno=3;
uint8 tsrpicslice=2;
uint8 adasreverved[4];
//

void printRawData(const std::vector<std::uint8_t>& data)
{
    std::cout << std::hex;
    std::copy(data.begin(), data.end(), std::ostream_iterator<unsigned>(std::cout, " "));
    std::cout << std::dec << '\n';
}

void UtcBcdTime(char* utc_buf)
{
    time_t lt=0;
    lt = time(NULL);
    struct tm * tmp_localtime = localtime(&lt);
    utc_buf[0]=(unsigned char)((((tmp_localtime->tm_year-100)/10)<<4)|((tmp_localtime->tm_year-100)%10));
    utc_buf[1]=(unsigned char)((((1+tmp_localtime->tm_mon)/10)<<4)|((1+tmp_localtime->tm_mon)%10));
    utc_buf[2]=(unsigned char)((((tmp_localtime->tm_mday)/10)<<4)|((tmp_localtime->tm_mday)%10));
    utc_buf[3]=(unsigned char)((((tmp_localtime->tm_hour)/10)<<4)|((tmp_localtime->tm_hour)%10));
    utc_buf[4]=(unsigned char)((((tmp_localtime->tm_min)/10)<<4)|((tmp_localtime->tm_min)%10));
    utc_buf[5]=(unsigned char)((((tmp_localtime->tm_sec)/10)<<4)|((tmp_localtime->tm_sec)%10));
}


Connection::Connection(io_service& io_service, ConnectionManager& manager
                      ) :socket_(io_service),
    connectionManager_(manager),
    _strand(io_service),
    heartbeat_timer_(io_service),
    mediadelay_timer_(io_service)
{ }

ip::tcp::socket& Connection::socket()
{
    return socket_;
}
void Connection::Stop()
{
    printf("connection stopped \n");
    //https://www.gamedev.net/blogs/entry/2249317-a-guide-to-getting-started-with-boostasio/
    boost::system::error_code ec;
    if(socket_.is_open())
    {
        //io_service.reset();
        //io_servicepost([this]() { Connection.Stop(); });
        socket_.shutdown( boost::asio::ip::tcp::socket::shutdown_both, ec );
        socket_.close(ec);
    }
    heartbeat_timer_.cancel();
    mediadelay_timer_.cancel();

}

void Connection::start()
{
    data_.reset(new std::vector<uint8_t>(3));
    auto self(shared_from_this());
    async_read(socket_, buffer(*data_), transfer_at_least(3),
               [this, self](const boost::system::error_code& ec, size_t /* bytes_transferred */)
    {
        if (!ec)
        {

        }
        else
        {
            //LOG_ERROR << "Network (" << error.default_error_condition().value() << ") " << error.default_error_condition().message() << std::endl;
            connectionManager_.Stop(self);
        }
    });
}

void Connection::Start()
{
    total_bytes_read=0;
    const unsigned int MESSAGESIZE=128*1024;
    buf.reset(new char[MESSAGESIZE]);
    auto self(shared_from_this());
    buf_size=MESSAGESIZE;

    readmedia(0);
    start_write();

}


//voif readmeia
void Connection::readmedia(std::size_t bytesCount)
{


    total_bytes_read+=bytesCount;

    // async_read(socket_,
    socket_.async_read_some(
        boost::asio::buffer(buf.get()+total_bytes_read,buf_size-total_bytes_read),
        [this](const boost::system::error_code& ec, std::size_t bytesCount)
    {
        if (ec == boost::asio::error::operation_aborted)
        {
            return;
        }

        if (ec)
        {
            std::cerr << "ERROR: Failed to read with error: " << ec.message() << std::endl;
            connectionManager_.Stop(getptr());
            return;
        }


        uint8 m_lastbyte=buf[total_bytes_read+bytesCount-1];        //find 0x7e

        //headtailpos =17
        //printf("total_bytes_read  is %d,last byte is 0x%2x\n",total_bytes_read,m_lastbyte);
        //check last byte
        if((m_lastbyte==0x7E)||(m_lastbyte==0x7e))
        {
            readpacket.clear();
            readpacket._storage.insert(readpacket._storage.end(), buf.get(), buf.get()+ total_bytes_read+bytesCount);
            //printf("origianl cnt is %d\n",readpacket.size());
            //readpacket.hexlike(true);
            readpacket.jt808decode();
            // printf("decode cnt is %d\n",readpacket.size());
            //readpacket.hexlike(true);
            readpacket>>s_jt808sync>>s_checksum>>s_sequencemsgcnt>>s_manufactureID>>s_terminalcodeID>>s_opcode;
            printf("sync is 0x%2x,terminalcodeID is 0x%2x ,opcode is 0x%2x \n",s_jt808sync,s_terminalcodeID,s_opcode);
            processInput();
            //restart receivesome
            bytesCount=0;
            total_bytes_read=0;
            //buf_size=MESSAGESIZE;
            total_bytes_read=0;
        }
        readmedia(bytesCount);
    });

}

//0x51
void Connection::handlattachmentload(WorldPacket &)
{
    //printf("handlel ******  0x51     *****armdata \n");
    mediadelay_timer_.cancel();
#define splitsize 600
    readpacket>>m_mediatype>>m_multimediaID;
    m_multimediaID=swap_endian<uint32_t>(m_multimediaID);
    readpacket>>m_totalpacketNo>>m_currentpacketNo;
    m_totalpacketNo=swap_endian<uint16_t>(m_totalpacketNo);
    m_currentpacketNo=swap_endian<uint16_t>(m_currentpacketNo);
    readpacket.clearheader();

    // printf("m_multimediaID is 0x%2x \n",m_multimediaID);
    // printf("m_totalpacketNo is 0x%2x ,m_currentpacketNo is 0x%2x \n",m_totalpacketNo,m_currentpacketNo);

    fwrite(readpacket.contents(),sizeof(uint8),readpacket.size(),m_file);

    uint16 packcnt=m_currentpacketNo+1;
    uint16 totalpacketNo=m_totalpacketNo;
    sendattachmentload_ack();
    if(totalpacketNo==packcnt)
    {
        //printf("end fwrite\n");

        fclose(m_file);//zlyyyyyyy
       // sync();
       // sync();
        m_file=NULL;

        // start_getalarmsmedia();//for pic delay 200ms
        sendmultimediarequest();//get next media
    }
    //start timer

}

void Connection::processInput()
{
    switch (s_opcode)
    {
    case 0x2F:
        handlecheckpacket_ack();
        break;
    case 0x30:
        handlerestorepacket_ack();
        break;
    case 0x32:
        handlereadconfigpacket_ack();
        break;
    case 0x35:
        handleactivesetconfigpacket_ack();
        break;
    case 0x34:
        handleactivesetconfigpacket();
        break;
    case 0x38://sensor status
        handlestatusreport(readpacket);
        break;
    case 0x36://alarm report
        handlealarmreport(readpacket);
        break;
    case 0x50://alarm report
        handlelarmdata(readpacket);
        break;
    case 0x51://alarm report
        //printf("********bytecnt  is %d \n",readpacket.size());
        // mediapacket._storgr
        handlattachmentload(readpacket);
        break;
    case 0x52://photo now ack report
        handlphotonowack();
        break;
    default:
        printf("********bytecnt  is %d \n",readpacket.size());
        printf("default handle opcode is 0x%2x\n",s_opcode);
        break;

    }
}

void Connection::start_write()
{
    if(socket_.is_open())
        sendrealtimepacket();
    heartbeat_timer_.expires_from_now(boost::posix_time::seconds(8));
    heartbeat_timer_.async_wait(_strand.wrap(boost::bind(&Connection::heart_tick,this,_1)));
}


//void Connection::heart_tick(const boost::system::error_code& ec)
void Connection::heart_tick(const boost::system::error_code& ec )
{

    auto self(shared_from_this());

    if (!ec)
    {
        if(socket_.is_open())
            sendrealtimepacket();
        // Wait 10 seconds before sending the next heartbeat.
        heartbeat_timer_.expires_from_now(boost::posix_time::seconds(8));
        heartbeat_timer_.async_wait(_strand.wrap(
                                        boost::bind(&Connection::heart_tick, shared_from_this(),_1)));
    }
    else
    {
        std::cout << "Error on heartbeat: " << ec.message() << "\n";
        connectionManager_.Stop(self);

        //stop();
    }
}
void Connection::packetwrite(WorldPacket& m_packet)
{
    if(socket_.is_open())
        _strand.post(
            boost::bind(
                &Connection::packetwrite_impl,
                shared_from_this(),
                m_packet
            )
        );

}

void Connection::packetwrite()
{
    const WorldPacket& message = _outbox[0];
    // message.hexlike(true);
    if(socket_.is_open())
        boost::asio::async_write(
            socket_,
            boost::asio::buffer( message.contents(), message.size() ),
            _strand.wrap(
                boost::bind(
                    &Connection::writeHandler,
                    shared_from_this(),
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred
                )
            )
        );
}
void Connection::packetwrite_impl(WorldPacket& m_packet)
{
    _outbox.push_back( m_packet );
    if ( _outbox.size() > 1 )
    {
        // outstanding async_write
        return;
    }

    this->packetwrite();
}

void Connection::writeHandler( const boost::system::error_code& error, const size_t bytesTransferred)
{
    _outbox.pop_front();
    auto self(shared_from_this());

    if ( error )
    {
        std::cerr << "could not write: " << boost::system::system_error(error).what() << std::endl;
        connectionManager_.Stop(self);
        return;
    }

    if ( !_outbox.empty() )
    {
        // more messages to send
        this->packetwrite();
    }
}

//check sersor status
//opcode 0x2F
void Connection::sendcheckpacket()
{
    WorldPacket checkpacket(0x2F,100);
    checkpacket<<sequencemsgcnt<<manufactureID<<s_terminalcodeID;
    checkpacket<<checkpacket.GetOpcode();
    checkpacket.jt808encode();	//printf("\n");
    // checkpacket.hexlike(true);
    packetwrite(checkpacket);

    printf("\n\n");
}

//0x2F ack
void Connection::handlecheckpacket_ack()
{
    printf("checkpacket_ack**********\n");
}

//restore default config packet
//opcaode 0x30
void Connection::sendrestorepacket()
{
    WorldPacket restorepacket(0x30,100);
    restorepacket<<sequencemsgcnt<<manufactureID<<s_terminalcodeID;
    restorepacket<<restorepacket.GetOpcode();
    restorepacket.jt808encode();	//printf("\n");
    //restorepacket.hexlike(true);
    packetwrite(restorepacket);
    printf("\n\n");
}

//0x30 ack
void Connection::handlerestorepacket_ack()
{
    printf("handlerestorepacket_ack**********\n");
}

//realtimepacket
//opcode 0x31
void Connection::sendrealtimepacket()
{
    int i;
    char utc_buf[6] = {0};
    uint8 car_speed=0x40;
    uint8 res1=0x00;//yuliu1
    uint32 lichen=swap_endian<uint32_t>(0x12345678);
    uint16 res2=0x00;
    uint16 height=swap_endian<uint16_t>(0x100);
    uint32 weidu=swap_endian<uint32_t>(0x23232323);
    uint32 jindu=swap_endian<uint32_t>(0x56125612);
//BCD TIME code 6
    uint16 carstate=swap_endian<uint16_t>(0x10);
    WorldPacket realtimedata(0x31,100);
    realtimedata.clear();
    sequencemsgcnt=swap_endian<uint16_t>(sequencemsgcnt);
    //make header
    realtimedata<<sequencemsgcnt<<manufactureID<<s_terminalcodeID;
    realtimedata<<realtimedata.GetOpcode();
    //headerend
    realtimedata<<car_speed<< res1 <<lichen <<res2 <<height<<weidu<<jindu;

    UtcBcdTime(utc_buf);
    for(  i=0; i<sizeof(utc_buf); i++)
    {
        //printf("%02x",utc_buf[i]);
        realtimedata<<uint8(utc_buf[i]);
    }
    //printf("0x31******************************\n");
    //realtimedata.hexlike(true);printf("\n");
    realtimedata<<carstate;
    realtimedata.jt808encode();	//printf("\n");
    //delay for PIC 200ms
    //delay for video 5s

    packetwrite(realtimedata);

}


//read sensor config
//opcode 0x32
void Connection::sendreadconfigpacket()
{
    WorldPacket readconfigpacket(0x32,100);
    readconfigpacket<<sequencemsgcnt<<manufactureID<<s_terminalcodeID;
    readconfigpacket<<readconfigpacket.GetOpcode();
    readconfigpacket.jt808encode();	//printf("\n");
    //readconfigpacket.hexlike(true);
    packetwrite(readconfigpacket);
    printf("\n\n");
    //m_socket.send(boost::asio::buffer(restorepacket.contents(),restorepacket.size()));
}

//0x32 ack
void Connection::handlereadconfigpacket_ack()
{
    printf("handlereadconfigpacket_ack**********\n");
}

//read active config
//opcaode 0x33
void Connection::sendactiveconfigpacket()
{
    WorldPacket activeconfigpacket(0x33,100);
    activeconfigpacket<<sequencemsgcnt<<manufactureID<<s_terminalcodeID;
    activeconfigpacket<<activeconfigpacket.GetOpcode();
    activeconfigpacket.jt808encode();	//printf("\n");
    //activeconfigpacket.hexlike(true);
    //printf("\n\n");
    packetwrite(activeconfigpacket);

    //m_socket.send(boost::asio::buffer(activeconfigpacket.contents(),activeconfigpacket.size()));
}
//firware update packet
//opcode0x33
//omit

//opcode 0x34
//
void  Connection::sendactivequerypacket()
{
    WorldPacket activequerypacket(0x34,100);
    //header
    activequerypacket<<sequencemsgcnt<<manufactureID<<s_terminalcodeID;
    activequerypacket<<activequerypacket.GetOpcode();
    activequerypacket.jt808encode();
    packetwrite(activequerypacket);
}


//opcode 0x34
//response
void  Connection::handleactivesetconfigpacket()
{

    if(s_terminalcodeID==ADASID)
        handleADASactivesetconfigpacket();
    if(s_terminalcodeID==DMSID)
        handleDSMactivesetconfigpacket();
}

void  Connection::handleADASactivesetconfigpacket()
{

    //header
    //readpacket>>sequencemsgcnt>>manufactureID>>s_terminalcodeID;
    //readpacket>>activesetconfigpacket.GetOpcode();
    //all
    readpacket>>speedthreshold>>volumn;
    readpacket>>activephoestagy;
    readpacket>>timedslice;//
    timedslice=swap_endian<uint16_t>(timedslice);
    readpacket>>distanceslice;//
    distanceslice=swap_endian<uint16_t>(distanceslice);//
    readpacket>>picturequality>>pictureslice>>picresolution>>videoresolution;
    //activesetconfigpacket>>reschach;//9
    readpacket.read_skip(9);//temporyly not read
    //obstacle
    readpacket>>obstacledistance>>obstaclevideotime>>obstaclepicsno>>obstaclepicslice;
    //change line frequently
    //24-28
    readpacket>>departuretime>>departurecnt>>departurevideotime;
    readpacket>>departurepicsno>>departurepicslice;
    //ldw  29-31
    readpacket>>ldwvideotime>>ldwpicsno>>ldwpicslice;
    //fcw 32-35
    readpacket>>fcwtime>>fcwvideotime>>fcwpicsno>>fcwpicslice;
    //pcw 36-39
    readpacket>>pcwtime>>pcwvideotime>>pcwpicsno>>pcwpicslice;
    //car too close 40-43
    readpacket>>carclosetime>>carclosevideotime>>carclosepicsno>>carclosepicslice;
    //tsr  44 45
    readpacket>>tsrpicsno>>tsrpicslice;
    //46 bytes[4]
    //adasreverved;
    readpacket.read_skip(4);//temporyly not read


}
//0x34 ack
void  Connection::handleDSMactivesetconfigpacket()
{

    //header
    //readpacket>>sequencemsgcnt>>manufactureID>>s_terminalcodeID;
    //readpacket>>activesetconfigpacket.GetOpcode();
    //all DMS 0-1
    readpacket>>speedthreshold>>volumn;

    readpacket>>activephoestagy;
    readpacket>>timedslice;//
    timedslice=swap_endian<uint16_t>(timedslice);
    readpacket>>distanceslice;//
    distanceslice=swap_endian<uint16_t>(distanceslice);//



    readpacket>>picturequality>>pictureslice>>picresolution>>videoresolution;
    //activesetconfigpacket>>reschach;//10
    //11 reserved bytes[10]
    readpacket.read_skip(10);//temporyly not read
//smoking detection time peroid

    readpacket>>smokingtime>>calltime;//
    smokingtime=swap_endian<uint16_t>(smokingtime);
    calltime=swap_endian<uint16_t>(calltime);//
    //drown 25-28
    readpacket>>drownvideotime>>drownpicsno>>drownpicslice>>reschar;
    //call 29-31
    readpacket>>callvideotime>>callpicsno>>callpicslice;
    //smoking 32-34
    readpacket>>smokingvideotime>>smokingpicsno>>smokingpicslice;
    //distraction 35 -37
    //***************************************
    readpacket>>distractionvideotime>>distractionpicsno>>distractionpicslice;
    //driver abnormal 38-40
    readpacket>>abnormalvideotime>>abnormalpicsno>>abnormalpicslice;

    // activesetconfigpacket>>keepit;
    //reserved two bytes
    readpacket.read_skip(2);//temporyly not read

    //packetwrite(activesetconfigpacket);
}

//query active safety system config
//opcode 0x35
void  Connection::sendactivesetconfigpacket()
{

    if(s_terminalcodeID==ADASID)
        sendADASactivesetconfigpacket();
    if(s_terminalcodeID==DMSID)
        sendDSMactivesetconfigpacket();

    // m_socket.send(boost::asio::buffer(activesetconfigpacket.contents(),activesetconfigpacket.size()));
}


void  Connection::sendADASactivesetconfigpacket()
{
    WorldPacket activesetconfigpacket(0x35,100);
    //header
    activesetconfigpacket<<sequencemsgcnt<<manufactureID<<s_terminalcodeID;
    activesetconfigpacket<<activesetconfigpacket.GetOpcode();
    //all
    activesetconfigpacket<<speedthreshold<<volumn;
    activesetconfigpacket<<activephoestagy;
    activesetconfigpacket<<swap_endian<uint16_t>(timedslice);//
    activesetconfigpacket<<swap_endian<uint16_t>(distanceslice);//
    activesetconfigpacket<<picturequality<<pictureslice<<picresolution<<videoresolution;
    //activesetconfigpacket<<reschach;//9
    for(auto i=0; i<9; i++)
    {
        activesetconfigpacket<<reschar;
    }
    //obstacle
    activesetconfigpacket<<obstacledistance<<obstaclevideotime<<obstaclepicsno<<obstaclepicslice;
    //change line frequently
    //24-28
    activesetconfigpacket<<departuretime<<departurecnt<<departurevideotime;
    activesetconfigpacket<<departurepicsno<<departurepicslice;
    //ldw  29-31
    activesetconfigpacket<<ldwvideotime<<ldwpicsno<<ldwpicslice;
    //fcw 32-35
    activesetconfigpacket<<fcwtime<<fcwvideotime<<fcwpicsno<<fcwpicslice;
    //pcw 36-39
    activesetconfigpacket<<pcwtime<<pcwvideotime<<pcwpicsno<<pcwpicslice;
    //car too close 40-43
    activesetconfigpacket<<carclosetime<<carclosevideotime<<carclosepicsno<<carclosepicslice;
    //tsr  44 45
    activesetconfigpacket<<tsrpicsno<<tsrpicslice;
    //46 bytes[4]
    //adasreverved;
    for(auto i=0; i<4; i++)
    {
        activesetconfigpacket<<reschar;
    }
    activesetconfigpacket.jt808encode();
    packetwrite(activesetconfigpacket);

}
//0x35
void Connection::sendDSMactivesetconfigpacket()
{
    WorldPacket activesetconfigpacket(0x35,100);
    //header
    activesetconfigpacket<<sequencemsgcnt<<manufactureID<<s_terminalcodeID;
    activesetconfigpacket<<activesetconfigpacket.GetOpcode();
    //all DMS 0-1
    activesetconfigpacket<<speedthreshold<<volumn;

    activesetconfigpacket<<activephoestagy;
    activesetconfigpacket<<swap_endian<uint16_t>(timedslice);//
    activesetconfigpacket<<swap_endian<uint16_t>(distanceslice);//
    activesetconfigpacket<<picturequality<<pictureslice<<picresolution<<videoresolution;
    //activesetconfigpacket<<reschach;//10
    //11 reserved bytes[10]
    for(auto i=0; i<10; i++)
    {
        activesetconfigpacket<<reschar;
    }
//smoking detection time peroid
    activesetconfigpacket<<swap_endian<uint16_t>(smokingtime)<<swap_endian<uint16_t>(calltime);//
    //drown 25-28
    activesetconfigpacket<<drownvideotime<<drownpicsno<<drownpicslice<<reschar;
    //call 29-31
    activesetconfigpacket<<callvideotime<<callpicsno<<callpicslice;
    //smoking 32-34
    activesetconfigpacket<<smokingvideotime<<smokingpicsno<<smokingpicslice;
    //distraction 35 -37
    //***************************************
    activesetconfigpacket<<distractionvideotime<<distractionpicsno<<distractionpicslice;
    //driver abnormal 38-40
    activesetconfigpacket<<abnormalvideotime<<abnormalpicsno<<abnormalpicslice;

    // activesetconfigpacket<<keepit;
    //reserved two bytes

    for(auto i=0; i<2; i++)
    {
        activesetconfigpacket<<reschar;
    }
    activesetconfigpacket.jt808encode();
    packetwrite(activesetconfigpacket);

}

//0x35 ack
void Connection::handleactivesetconfigpacket_ack()
{
    printf("handleactivesetconfigpacket_ack**********\n");
}

//set adas dms BOD config
//opcode 0x35
void Connection::sendadasconfigpacket()
{
    WorldPacket adasconfigpacket(0x35,100);
    adasconfigpacket<<sequencemsgcnt<<manufactureID<<s_terminalcodeID;
    adasconfigpacket<<adasconfigpacket.GetOpcode();
    adasconfigpacket.jt808encode();	//printf("\n");
    //adasconfigpacket.hexlike(true);
    packetwrite(adasconfigpacket);
    //  m_socket.send(boost::asio::buffer(adasconfigpacket.contents(),adasconfigpacket.size()));
    printf("sendadasconfigpacket\n\n");
}
//alarm report uplink
//opcode 36

//check sensorstatus
//opcode 0x37
void Connection::sendchecksensorstatuspacket()
{
    WorldPacket checksensorstatuspacket(0x35,100);
    checksensorstatuspacket<<sequencemsgcnt<<manufactureID<<s_terminalcodeID;
    checksensorstatuspacket<<checksensorstatuspacket.GetOpcode();
    checksensorstatuspacket.jt808encode();	//printf("\n");
    checksensorstatuspacket.hexlike(true);
    packetwrite(checksensorstatuspacket);
    //    m_socket.send(boost::asio::buffer(checksensorstatuspacket.contents(),checksensorstatuspacket.size()));
    printf("sendchecksensorstatuspacket\n\n");
}
//op code =38
void Connection::handlesensorstatus()
{

}
void Connection::start_getalarmsmedia(int mediatype)
{
    printf("start_getalarmsmedia\n");
    mediarequestcnt=0;
    //sendmultimediarequest();//get next media
    //sendcurrentmultimediarequest();
    if(mediatype==0x00)
        mediadelay_timer_.expires_from_now(boost::posix_time::milliseconds(200));
    if(mediatype==0x02)
        mediadelay_timer_.expires_from_now(boost::posix_time::seconds(8));

    mediadelay_timer_.async_wait(_strand.wrap(boost::bind(&Connection::getalarmsmedia,shared_from_this(),_1)));
}
void Connection::getalarmsmedia(const boost::system::error_code& ec )
{
    sendcurrentmultimediarequest();//get next media
//   mediarequestcnt++;
    //  if(mediarequestcnt<5)
    {
        //      mediadelay_timer_.expires_from_now(boost::posix_time::seconds(3));
        //      mediadelay_timer_.async_wait(_strand.wrap(boost::bind(&Connection::getalarmsmedia,shared_from_this(),_1)));
    }
//   else
    {
        mediadelay_timer_.cancel();
        // sendmultimediarequest();
    }
}



//opcode 50
//request media data
void Connection::sendcurrentmultimediarequest()
{
    WorldPacket multimediarequest(0x50,100);
    multimediarequest<<sequencemsgcnt<<manufactureID<<s_terminalcodeID;
    multimediarequest<<multimediarequest.GetOpcode();
    multimediarequest<<t_mediainfodescription.mediatype;
    multimediarequest<<swap_endian<uint32_t>(t_mediainfodescription.multimediaID);
    multimediarequest.jt808encode();	//printf("\n");
    // multimediarequest.hexlike(true);
    packetwrite(multimediarequest);
}

void Connection::sendmultimediarequest()
{

    int mediatype;

    if(!m_mediainfodescription.empty())
    {
        printf("send ****0x50***** multimediarequest\n");
        t_mediainfodescription=m_mediainfodescription.front();

        WorldPacket multimediarequest(0x50,100);
        multimediarequest<<sequencemsgcnt<<manufactureID<<s_terminalcodeID;
        multimediarequest<<multimediarequest.GetOpcode();
        multimediarequest<<t_mediainfodescription.mediatype;
        multimediarequest<<swap_endian<uint32_t>(t_mediainfodescription.multimediaID);
        m_mediainfodescription.pop_front();
        mediatype=t_mediainfodescription.mediatype;
        char       mediafile[80];
        char       mediafilename[160];
        char    tmptype[128];
        char tmp[80];

        struct timeb tb;

        ftime(&tb);
        strftime(tmp,sizeof(tmp),"%Y%m%d_%H%M_%S_",localtime(&tb.time));//Year Month Day Hour Minute
        sprintf(tmptype,"%s%03d",tmp,tb.millitm);//time+ms

        switch (t_mediainfodescription.mediatype)
        {
        case 0x00://sensor status
            sprintf(mediafile,"%s.jpg",tmptype);
            break;
        case 0x02://alarm report
            sprintf(mediafile,"%s.mp4",tmptype);
            break;
        default:
            sprintf(mediafile,"%s.bin",tmptype);
            break;
        }
        /*
                if(s_terminalcodeID==ADASID)
                {

                    sprintf(mediafilename,"%s%s%",ADAS_PATH,mediafile);
                    ADAS_INFO("alarm file is "<<mediafilename);
                }
                if(s_terminalcodeID==DMSID)
                {

                    sprintf(mediafilename,"%s%s%",DMS_PATH,mediafile);
                    DMS_INFO("alarm file is "<<mediafilename);
                }
                */
        printf("%s\n",mediafile);

        //create file
        m_file = fopen(mediafile,"wb");
        // multimediarequest<<swap_endian<uint64_t>(mutimediaID);

        //multimediarequest.jt808encode();	//printf("\n");
        // multimediarequest.hexlike(true);
        // packetwrite(multimediarequest);
        start_getalarmsmedia(mediatype);
//   m_socket.send(boost::asio::buffer(multimediarequest.contents(),multimediarequest.size()));

    }
}
//0x51 ack to sensor
void Connection::sendmultimediaack()
{
    WorldPacket photenowpacket(0x51,100);
    photenowpacket<<sequencemsgcnt<<manufactureID<<s_terminalcodeID;
    photenowpacket<<photenowpacket.GetOpcode();
    photenowpacket.jt808encode();	//printf("\n");
    //photenowpacket.hexlike(true);
    //   m_socket.send(boost::asio::buffer(phonenowpacket.contents(),phonenowpacket.size()));
    packetwrite(photenowpacket);
    printf("photenowpacket\n\n");
}
//sensor status report
//opcode uplink

//photo right now
//opcode 0x52
void Connection::sendphotenowpacket()
{
    WorldPacket photenowpacket(0x52,100);
    photenowpacket<<sequencemsgcnt<<manufactureID<<s_terminalcodeID;
    photenowpacket<<photenowpacket.GetOpcode();
    photenowpacket.jt808encode();	//printf("\n");
    //photenowpacket.hexlike(true);
    //   m_socket.send(boost::asio::buffer(phonenowpacket.contents(),phonenowpacket.size()));
    printf("sendmultimediarequest\n\n");
    packetwrite(photenowpacket);
}



//0x38
void Connection::handlestatusreport(WorldPacket &)
{
    printf("0x38 processing \n");
    uint8 sensorstate;
    uint32 alarmstate;
    readpacket>>sensorstate>>alarmstate;
    switch (sensorstate)
    {
    case 0x01://sensor status
        printf("sensor normal\n");
        break;
    case 0x02://alarm report
        printf("sensor standby\n");
        break;
    case 0x03://alarm report
        printf("sensor upgrading,standbying\n");
        break;
    case 0x04://alarm report
        printf("sensor mulfunction\n");
        break;
    default:
        break;
    }
    sendactivesetconfigpacket();
}
//get alarm report multimeidaID
//send multimedia data request
void Connection::handlealarmreport(WorldPacket &)
{
    // printf("handlealarmreport\n");
    if(s_terminalcodeID==ADASID)
        handladasealarmreport(readpacket);
    if(s_terminalcodeID==DMSID)
        handledsmalarmreport(readpacket);



}
void Connection::handladasealarmreport(WorldPacket &)
{
    printf("handleadasalarmreport\n");
    uint8 m=0;
    uint32 alarmID;
    uint8 alarmstate;
    uint8 alarmType;

    uint8 frontcarspeed;
    uint8  m_distance;
    uint8 departuretype;
    uint8 tsrtype;
    uint8 tsrdata;
    uint8 mycarspeed;
    uint16 mycarheight;
    uint32 mycarweidu;
    uint32 mycarjindu;
    //uint8-6 myBCDTIME;
    int16 mycarstate;
    uint8 mediacnt;
    //alarm
    readpacket>>alarmID>>alarmstate>>alarmType;
    alarmID=swap_endian<uint32_t>(alarmID);
    printf("alarmID is 0x%2x,alarmstate is 0x%2x ,alarmType is 0x%2x \n",alarmID,alarmstate,alarmType);
    readpacket>>frontcarspeed>>m_distance>>departuretype;
    switch (departuretype)
    {
    case 0x01://drowningness status
        printf("left  departure \n");
        break;
    case 0x02://alarm report
        printf("right  departure \n");
    default:
        break;
    }
    //tsr
    readpacket>>tsrtype>>tsrdata;
    //mycar
    readpacket>>mycarspeed>>mycarheight>>mycarweidu>>mycarjindu;
    readpacket.read_skip(6);//temporyly not reat BCD time
    readpacket>>mycarstate>>mediacnt;
    switch (alarmType)
    {
    case 0x01://drowningness status
        printf("Front Collission \n");
        break;
    case 0x02://alarm report
        printf("lane  departure \n");
        break;
    case 0x03://smoking report
        printf("car too close \n");
        break;
    case 0x04://alarm report
        printf("Pedestrian Collision Warning\n");
        break;
    case 0x05://alarm report
        printf("driver mulfunction\n");
    case 0x06://alarm report
        printf("Over traffic sign\n");
        break;
    case 0x10://alarm report
        printf("traffic sign warning\n");
        break;
    case 0x11://alarm report
        printf("active photo captured\n");
        break;
    default:
        break;
    }
    printf("***************************************************\n");

    // printf("mediacnt is %d \n",mediacnt);
    readpacket.hexlike(true);
    // printf("mediacnt is %d \n",mediacnt);
    m_mediainfodescription.clear();
    mediainfodescription tm_mediainfodescription;
    //printf("0x36 media cnt is %d\n",mediacnt);
    for(m=0; m<mediacnt; m++)
    {
        readpacket>>tm_mediainfodescription.mediatype;
        readpacket>>tm_mediainfodescription.multimediaID;
        tm_mediainfodescription.multimediaID=swap_endian<uint32_t>(tm_mediainfodescription.multimediaID);
        m_mediainfodescription.push_back(tm_mediainfodescription);

    }
    //start send ox50 request
    //start a timer to collect alarm detachments files.
    //start_getalarmsmedia();
    sendmultimediarequest();
}


void Connection::handledsmalarmreport(WorldPacket &)
{
    printf("handledsmalarmreport\n");
    uint8 m=0;
    uint32 alarmID;
    uint8  alarmstate;
    uint8 alarmType;
    uint8 drownnesslevel;
    // uint8 reserved4;//reserve byte[4]
    uint8 mycarspeed;
    uint16 mycarheight;
    uint32 mycarweidu;
    uint32 mycarjindu;
    //uint8-6 myBCDTIME;
    int16 mycarstate;
    uint8 mediacnt;
    //alarm
    readpacket>>alarmID>>alarmstate>>alarmType>>drownnesslevel;
    alarmID=swap_endian<uint32_t>(alarmID);
    printf("alarmID is 0x%2x,alarmType is %d ,drownnesslevel is 0x%2x \n",alarmID,alarmType,drownnesslevel);
    readpacket.read_skip(4);
    readpacket>>mycarspeed>>mycarheight>>mycarweidu>>mycarjindu;
    readpacket.read_skip(6);//temporyly not read BCD time
    readpacket>>mycarstate>>mediacnt;
    switch (alarmType)
    {
    case 0x01://drowningness status
        printf("drowningness alarm\n");
        break;
    case 0x02://alarm report
        printf("call  alarm\n");
        break;
    case 0x03://smoking report
        printf("smoking alarm \n");
        break;
    case 0x04://alarm report
        printf("driver focus distraction\n");
        break;
    case 0x05://alarm report
        printf("driver mulfunction\n");
        break;
    default:
        break;
    }
    printf("***************************************************\n");
    readpacket.hexlike(true);
    // printf("mediacnt is %d \n",mediacnt);
    m_mediainfodescription.clear();
    mediainfodescription tm_mediainfodescription;
    //printf("0x36 media cnt is %d\n",mediacnt);
    for(m=0; m<mediacnt; m++)
    {
        readpacket>>tm_mediainfodescription.mediatype;
        readpacket>>tm_mediainfodescription.multimediaID;
        tm_mediainfodescription.multimediaID=swap_endian<uint32_t>(tm_mediainfodescription.multimediaID);
        m_mediainfodescription.push_back(tm_mediainfodescription);

    }
    //start send ox50 request
    //start a timer to collect alarm detachments files.
    //start_getalarmsmedia();
    sendmultimediarequest();

}
//
//zly
//0x50 respondse
//
void Connection::handlelarmdata(WorldPacket &)
{

    printf("hand 0x50 acked \n");//from sensor
}

//0x51 response
void Connection::sendattachmentload_ack()
{
    printf("sendattachmentload_ack \n");
    WorldPacket attachmentload_ack(0x51,100);
    uint8 m_receivedatasuccess=0;
    sequencemsgcnt++;
    //header
    attachmentload_ack<<sequencemsgcnt<<manufactureID<<s_terminalcodeID;
    //opcode
    attachmentload_ack<<attachmentload_ack.GetOpcode();//0xx51
    //mediatype
    attachmentload_ack<<m_mediatype;//00
    //multimediaID
    attachmentload_ack<<swap_endian<uint32_t>(m_multimediaID);//00 00 00 01
    //packet info
    m_totalpacketNo=swap_endian<uint16_t>(m_totalpacketNo);//00 01
    m_currentpacketNo=swap_endian<uint16_t>(m_currentpacketNo);//00 01
    attachmentload_ack<<m_totalpacketNo<<m_currentpacketNo<<m_receivedatasuccess;//
    attachmentload_ack.jt808encode();	//printf("\n");
    attachmentload_ack.hexlike(true);
    packetwrite(attachmentload_ack);

}

void Connection::handlphotonowack()
{
    printf("photo now acked\n");
}

void Connection::sendtiimepacket(WorldPacket  packet)
{
    // boost::asio::async_write(m_socket, boost::asio::buffer(packet.contents(), packet.size()),
    //                          boost::bind(&Connection::handle_write, this, _1));

}
void Connection::handle_writepacket(const boost::system::error_code& ec)
{


}


void Connection::sendpacket(WorldPacket  packet)
{
    //  boost::asio::async_write(m_socket, boost::asio::buffer(packet.contents(), packet.size()),
    //                           boost::bind(&Connection::handle_writepacket, this, _1));
    boost::asio::write(socket_,boost::asio::buffer(packet.contents(), packet.size()));

}


