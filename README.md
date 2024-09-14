# 윈도우 C++ 게임 서버 라이브러리

게임 서버 프로젝트 진행 중 라이브러리를 업데이트를 편하게 하기 위해 이 프로젝트를 공유 항목 프로젝트로 설정했습니다. 
다른 프로젝트의 서버를 컴파일하려면 이 레포지토리를 클론하고, 해당 프로젝트에 이 프로젝트를 참조로 추가해야 합니다.

## Network
IOCPServer

IOCPClient

IOCPDummyClient

## Buffer
CSendBuffer 

CRecvBuffer: RingBuffer를 직렬화 버퍼처럼 사용하게 해주는 클래스

CRingBuffer

SendBuffer는 선형 배열(Linear Array)을 사용하고 RecvBuffer는 RingBuffer의 참조를 사용합니다.

## Container
LockQueue

LockStack

MPSCQueue

## DB
MYSQLHelper

RedisHelper

## RoomSystem
Job

JobQueue

Room

RoomSytem

WorkThreadPool

Room은 내부 로직의 순차 처리를 보장합니다. 
각 세션이 하나의 RoomSystem 내에서 단 하나의 Room에만 존재하도록 보장합니다.

## Memory
TlsMemoryPool

TlsObjectPool

CommonPool: TlsMemoryPool을 Bucket 단위로 미리 생성하여 사용하는 Pool

## RPC
RPCcompiler2.0.py

## Profiler
PerformanceProfiler

CacheTracer

PerformanceMonitor

