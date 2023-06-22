#include <iostream>
#include <cstdlib>
#include "thread.h"
#include "cpu.h"
#include "mutex.h"

using std::cout;
using std::endl;

mutex m;
cv c;

void funcA(void* a) {
    m.lock();
    cout << "Child locked.\n";
    c.wait(m);
    cout << "Child past first wait.\n";
    c.wait(m);
    cout << "Child past second wait.\n";
    m.unlock();
}
void boot_func(void*) {
    cout << "Creationg Child.\n";
    thread t(funcA, nullptr);
    m.lock();
    cout << "Signalling Child.\n";
    c.signal();
    m.unlock();
    t.join();
    cout << "Parent Exiting.\n";
}
int main()
{
    cpu::boot((thread_startfunc_t)boot_func, (void*)0, 0);
    //cpu::boot(1, (thread_startfunc_t)boot_func, (void*)100, false, false, 0);
}
