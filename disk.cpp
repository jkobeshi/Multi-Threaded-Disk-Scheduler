#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include "thread.h"
#include "disk.h"

mutex Qm, Cm, Rm;
cv reqCv, serCv;

int max_disk_queue, queue_count = 0, req_count = 0;
std::vector<bool> waitQ;
std::vector<int> readyQ;
std::vector<std::ifstream> files;
std::vector<int> numTracksInFile;
std::vector<bool> booted;

int CountLiveReq() {
	int count = 0;
	for (int i = 0; i < (int)numTracksInFile.size(); ++i) {
		if ((numTracksInFile[i] != 0) && booted[i]) {
			++count;
		}
	}
	return count;
}

void servicer(void* a) {
	int trk = 0;
	while (true) {
		Qm.lock(); Rm.lock();	//We wait until we get the largest possible number of request
		while (((max_disk_queue <= req_count) && (queue_count != max_disk_queue)) || //when (max_disk_queue <= req_count) && (queue_count != max_disk_queue) then wait
			((max_disk_queue > req_count) && (queue_count != req_count)) || //when (max_disk_queue > req_count) && (queue_count != req_count) then wait
			(queue_count == 0)) {		 //when theres nothing in the queue then wait
			Rm.unlock(); serCv.wait(Qm); Rm.lock();
		} Rm.unlock();
		int req = 0, minDist = 1000;
		for (int i = 0; i < (int)files.size(); ++i) //handling shortest seek time first
			if (waitQ[i])
				if (minDist > abs(readyQ[i] - trk)) {
					minDist = abs(readyQ[i] - trk); req = i;
				}
		trk = readyQ[req]; //servicing
		Cm.lock(); print_service(req, trk); Cm.unlock();//printing
		--numTracksInFile[req];
		Rm.lock(); req_count = CountLiveReq(); Rm.unlock();
		waitQ[req] = false; --queue_count; //dequeue
		reqCv.broadcast(); Qm.unlock();
	}
}

void requester(void* a) {
	booted[(intptr_t)a] = true;
	Rm.lock(); req_count = CountLiveReq(); Rm.unlock();
	int track = 0;
	while (files[(intptr_t)a] >> track) {
		Qm.lock();
		while (max_disk_queue <= queue_count) //waiting until the queue is not full
			reqCv.wait(Qm);
		readyQ[(intptr_t)a] = track; waitQ[(intptr_t)a] = true; ++queue_count; //enqueue
		Cm.lock(); print_request((intptr_t)a, track); Cm.unlock(); //printing
		serCv.signal();
		while (waitQ[(intptr_t)a]) //cant go next until done servicing prev request
			reqCv.wait(Qm);
		Qm.unlock();
	}
	Rm.lock(); req_count = CountLiveReq(); Rm.unlock();
	serCv.signal();
}

void scheduler(void* a) {
	for (intptr_t i = 0; i < (intptr_t)files.size(); ++i) {
		thread reqT((thread_startfunc_t)requester, (void*)i);
	}
	thread serT((thread_startfunc_t)servicer, (void*)0);
}

int main(int argc, char* argv[]) {
	max_disk_queue = atoi(argv[1]);
	waitQ.resize(argc - 2, false);
	readyQ.resize(argc - 2);
	booted.resize(argc - 2, false);
	numTracksInFile.resize(argc - 2, 0);
	for (int i = 2; i < argc; ++i) {
		files.push_back(std::ifstream(argv[i]));
		if (!files.back().is_open())
			return 1;
	}
	std::vector<std::ifstream> Tfiles;
	for (int i = 2; i < argc; ++i)
		Tfiles.push_back(std::ifstream(argv[i]));
	for (int i = 0; i < (int)files.size(); ++i) {
		int temp;
		while (Tfiles[i] >> temp) {
			++numTracksInFile[i];
		}
	}
	cpu::boot((thread_startfunc_t)scheduler, (void*)0, 0);
	return 0;
}
