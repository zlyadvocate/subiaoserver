# subiaoserver
active safety protocol server of SUbiao

工程文件为codeblocks。
按苏标主动安全协议编写。支持心跳，心跳包由server定时发出。
图片和视频文件是一个包接受到的，有组包处理。
在接收报警时间的图片和视频数据期间，如果有新的报警事件上来会丢弃没有接收到的图片和视频。a
ADAS和DMS使用不同的套接字。主动抓拍时没有视频上传。


流水号没有使用。

依赖库：BOOST version V1.65，最新的版本也没有问题。
测试情况：和Minieye,瑞为等主动一体机进行过对接，测试通过。
