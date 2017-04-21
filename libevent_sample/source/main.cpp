#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <iostream>
#include <thread>
#include <atomic>

#ifndef WIN32
#include <netinet/in.h>
# ifdef _XOPEN_SOURCE_EXTENDED
#  include <arpa/inet.h>
# endif
#include <sys/socket.h>
#endif

#include <event2/thread.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/listener.h>
#include <event2/util.h>
#include <event2/event.h>
#include <event2/bufferevent_struct.h>

std::atomic_bool write_over;

void eventcb(struct bufferevent *bev, short events, void *ptr)
{
    if (events & BEV_EVENT_CONNECTED) 
    {
    	fprintf(stderr, "connected\n");
         /* We're connected to 127.0.0.1:8080.   Ordinarily we'd do
            something here, like start reading or writing. */
    } 
    else if (events & BEV_EVENT_ERROR) 
    {
    	fprintf(stderr, "error\n");
         /* An error occured while connecting. */
    }
}

//����Server������Ϣ
static void conn_readcb(struct bufferevent *bev, void *user_data)//arg1:�������¼���bufferevent�����һ�����û��ṩ�Ĳ���������ͨ�������ص����ݲ���
{
    char buf[50] = {0};
    int len = bufferevent_read(bev, buf, sizeof(buf));
    std::cout<<"����Server:"<< buf << std::endl;
}

static void conn_writecb(struct bufferevent *bev, void *user_data)
{
    write_over = true;
    fprintf(stderr, "conn_writecb\n");
    //int ret = bufferevent_write(bev, "����һ���ͻ��ˣ�", 20);
}
   
struct event_base *g_base;//�����¼�


int main()
{
    write_over = false;
    struct bufferevent *bev;
    struct sockaddr_in sin;
#ifdef WIN32
    WSADATA wsa_data;
    WSAStartup(0x0201, &wsa_data);
#endif
event_enable_debug_mode();

    //event֧�ֶ��̵߳ĳ�ʼ������
    evthread_use_pthreads();
    
    auto event_conf = event_config_new();
		event_config_set_flag(event_conf, EVENT_BASE_FLAG_EPOLL_USE_CHANGELIST);
		//event_config_set_num_cpus_hint(event_conf, get_cpus());
		g_base = event_base_new_with_config(event_conf);
		event_config_free(event_conf);

    //g_base = event_base_new();

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = htonl(0x7f000001); /* 127.0.0.1 */
    sin.sin_port = htons(9999); /* Port 9527 */

    bev = bufferevent_socket_new(g_base, -1, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_THREADSAFE | BEV_OPT_DEFER_CALLBACKS);//���������׽��ֵ�bufferevent
		bufferevent_setwatermark(bev, EV_READ | EV_WRITE, 0, 0);


    if (bufferevent_socket_connect(bev,//connect�׽���
        (struct sockaddr *)&sin, sizeof(sin)) < 0) 
    {
        /* Error starting connection */
        std::cout<<"����ʧ�ܣ�\n";
        bufferevent_free(bev);
        return -1;
    }
    
    bufferevent_setcb(bev, conn_readcb, conn_writecb, eventcb, NULL);//�޸Ļص�
    bufferevent_enable(bev, EV_READ | EV_WRITE | EV_TIMEOUT );

    
		evutil_socket_t fd = bufferevent_getfd(bev);
		int option = 1; int len = sizeof(long);
		int r = setsockopt(fd, IPPROTO_TCP, 1, (char*)&option, len);
		std::cout << "[network_engine]: bufferevent_socket_connect process, wait for accept!!!" << std::endl;
							
    std::thread th([]
    {
        event_base_dispatch(g_base);//ѭ�������¼�
    });
    
    
    
    int ret = bufferevent_write(bev, "���ǿͻ���-1", 20);

    std::cout<<"�������뷢�͵����ݣ�\n";
    int count = 0;
    while(1)
    {
    //    if(write_over)
        {
            write_over = false;
            char msg[50] = {0};
            sprintf(msg, "��%d�η�����Ϣ��", ++count);
            bufferevent_write(bev, msg, strlen(msg) + 1);
            //Sleep(500);
        }
    //    Sleep(50);
    }

    th.join();
    std::cout<<" over!\n";
    getchar();
    return 0;
}