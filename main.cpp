#include <iostream>
#include <iomanip>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string>
#include <cstring>
#include <memory>
#include <vector>

/////////////////////////////////function templates///////////////////////
template <typename Tchar>
void zeroMemory(Tchar buff,unsigned size){
    for(unsigned c{0};c<size;c++){
        buff[c] = '\0';
    }
}
/////////////////////////////////function templates///////////////////////


///////////////////////////////////////user defined types///////////////////////

struct driveInfo{
        char root;
        unsigned lpSectorsPerCluster;
        unsigned lpBytesPerSector;
        unsigned lpNumberOfFreeClusters;
        unsigned lpTotalNumberOfClusters;

        unsigned long long lpFreeBytesAvailableToCaller;
        unsigned long long lpTotalNumberOfBytes;
        unsigned long long lpTotalNumberOfFreeBytes;
}__attribute__((packed));

//this is a struct to serialize and send over the network
//it doesn't include the drive info as that will be sent separately
struct clientSystemInfo{
    //os info
    char hostName[25];
    char osVersion[50];
    unsigned osBuild[3];
    
    //processor info
    char cpuBitNum[15];
    unsigned coreCount;
    unsigned threadCount;
    char manufacturer[13];
    char cpuModelStr[75];

    //memory info
    double totalMemory;
    double availableMemory;
    unsigned percentInUse;

    clientSystemInfo(){
        zeroMemory<char*>(hostName,25);
        zeroMemory<char*>(osVersion,50);
        zeroMemory<char*>(cpuBitNum,15);
        zeroMemory<char*>(manufacturer,13);
        zeroMemory<char*>(cpuModelStr,75);
    }
}__attribute__((packed));
///////////////////////////////////////user defined types///////////////////////

///////////////////////////////////////Function Declarations///////////////////////
void printFormat(std::string string);
///////////////////////////////////////Function Declarations///////////////////////

int main(){
    //Create a socket
    int sSock{socket(AF_INET,SOCK_STREAM,0)};//server socket(file descriptor)
    if (sSock == -1){
        std::cerr<<"Failed to create a socket";
        return -1;
    }
    
    //Bind the socket to an IP / port
    sockaddr_in sSockAddr;
        sSockAddr.sin_family = AF_INET;
        sSockAddr.sin_port = htons(62211);
        inet_pton(AF_INET,"192.168.56.2",&sSockAddr.sin_addr);
    if (bind(sSock,reinterpret_cast<sockaddr*>(&sSockAddr),sizeof(sSockAddr))== -1){
        std::cerr<<"Failed to bind Socket to IP/Port";
        return -2;
    }

    //Mark the socket for listening in
    if(listen(sSock,SOMAXCONN)==-1){
        std::cerr<<"Failed to set socket as server(listening)";
        return -3;
    }

    //Accept a call
    sockaddr_in cSockAddr;
    socklen_t cSockAddrSize{sizeof(cSockAddr)};
    char host[NI_MAXHOST]{};
    char svc[NI_MAXSERV]{};
    //client socket for actual communication - does not affec the sSock (listening socket)
    int cSock{
        accept(sSock,
        reinterpret_cast<sockaddr*>(&cSockAddr),
        &cSockAddrSize)
    };
    if (cSock == -1){
        std::cerr<<"Failed to connect to client!";
        return -4;
    }
    
    std::unique_ptr<char> sysData{new char[sizeof(clientSystemInfo)]};
    std::unique_ptr<char> driveData{new char[sizeof(driveInfo)]};
    char* DIVPtr{nullptr}; driveInfo* driveInfoArray{nullptr};
    char* driveCountBuff{new char[4]};unsigned* driveCount{nullptr};
    unsigned DIAI{0};//drive info array index
    unsigned bytesRecv;
    for (unsigned c{0};c<3;++c){
        if(c==0){
            bytesRecv = recv(cSock,sysData.get(),218,0);
        }else if(c==1){
            bytesRecv = recv(cSock,driveCountBuff,4,0);
            driveCount = reinterpret_cast<unsigned*>(driveCountBuff);
            DIVPtr = new char[(sizeof(driveInfo)*(*driveCount))];
            //driveInfoVec.resize(*driveCount);
        }
        else if (c==2){
            bytesRecv = recv(cSock,DIVPtr,((sizeof(driveInfo))*(*driveCount)),0);
        }
    }
    clientSystemInfo* sysDataCSI = reinterpret_cast<clientSystemInfo*> (sysData.get());
    driveInfoArray = reinterpret_cast<driveInfo*>(DIVPtr);
    //send(cSock,buf,bytesRecv+1,0);
    //Close sockets
    close(cSock);close(sSock);
    std::cout<<sysDataCSI->hostName<<'\n'<<sysDataCSI->osVersion<<'\n'<<
    sysDataCSI->osBuild[0]<<'.'<<sysDataCSI->osBuild[1]<<'.'<<sysDataCSI->osBuild[2]
    <<'\n'<<std::endl
    <<sysDataCSI->cpuBitNum<<'\n'<<sysDataCSI->coreCount<<'\n'<<sysDataCSI->threadCount
    <<'\n'<<sysDataCSI->manufacturer<<'\n'<<sysDataCSI->cpuModelStr
    <<'\n'<<std::endl<<sysDataCSI->totalMemory<<'\n'<<sysDataCSI->availableMemory
    <<'\n'<<sysDataCSI->percentInUse<<"% in use"<<'\n'<<std::endl;

    printFormat("Drive");printFormat("Total Bytes");printFormat("Available Bytes");
    std::cout<<'\n';
    for(unsigned c{0};c<*driveCount;++c){
        std::cout<<std::setw(15)<<std::left<<driveInfoArray[c].root<<
        std::setw(15)<<std::right<<driveInfoArray[c].lpTotalNumberOfBytes
        <<std::setw(15)<<std::right<<driveInfoArray[c].lpTotalNumberOfFreeBytes<<'\n';
    }
    delete [] driveCountBuff; delete [] DIVPtr;
    return 0;
}

void printFormat(std::string string){
    std::cout.width(15);
    std::cout<<std::left<<string;
}