# Memory Coordinator

Please write your algorithm and logic for the memory coordinator in this readme. Include details about how the algorithm is designed and explain how your implementation fulfills the project requirements. Use pseudocode or simple explanations where necessary to make it easy to understand.

In each iteration:
1. Query memory stats of each domain using `virDomainMemoryStats`. To be specific, we want to get the unused memory, available memory, and balloon memory of each domain.
2. Categorize domains into three classes, first, domains with extra unused memory(i.e. larger than the max unused memory threshold), second, domains with insufficient unused memory(i.e. smaller than the min unused memory threshold), and the last one, domains with enough memory. At the same time, we can calculate total memory we need by multiplying the number of the second class with the size of the memory transfer unit(100MB in my implementation) and total memory we can release by similiarly multiplying the number of the first class with the unit size.
3. If the total required memory is larger than total available memory, we need to request memory from the host. Therefore, we have to check if the host has enough free memory to be able to allocate.
4. Begin modifying balloon size of each VM. Specifically, if the total available memory is larger than or equal to the required memory, we just iterate the whole domain list and release extra memory and allocate needed memory. Otherwise, we release all the extra memory and allocate memory for as many VMs as possible.