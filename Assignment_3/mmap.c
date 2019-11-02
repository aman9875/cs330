// Aman Tiwari , Roll No - 160094

#include<types.h>
#include<mmap.h>

/**
 * Function will invoked whenever there is page fault. (Lazy allocation)
 * 
 * For valid acess. Map the physical page 
 * Return 1
 * 
 * For invalid access,
 * Return -1. 
 */
int vm_area_pagefault(struct exec_context *current, u64 addr, int error_code)
{		
	int fault_fixed = -1;   
	struct vm_area *vm_area_head = current->vm_area;
	int flag = 0;
	void *base_addr;
	base_addr = osmap(current->pgd);

	while(vm_area_head != NULL){
		if(vm_area_head->vm_start <= addr && vm_area_head->vm_end > addr){
			flag = 1;
			break;
		}
		vm_area_head = vm_area_head->vm_next;
	}

	if(!flag){
		return fault_fixed;
	} else {
		long diff = addr - vm_area_head->vm_start;
		long addr1 = addr - (diff % PAGE_SIZE);
		long prot = vm_area_head->access_flags;
		if(error_code == 0x6){ // unmapped write
			if(vm_area_head->access_flags & PROT_WRITE){
				u64 pfn = map_physical_page((u64) base_addr, addr1, prot, 0);
				fault_fixed = 1;
			}
		} else if(error_code == 0x4){ // unmapped read
			if(vm_area_head->access_flags & PROT_READ || vm_area_head->access_flags & PROT_WRITE){
				u64 pfn = map_physical_page((u64) base_addr, addr1, prot, 0);
				fault_fixed = 1;
			}
		}
	}
    return fault_fixed;
}

void merge_nodes(struct exec_context *current){
	struct vm_area *vm_area_head = current->vm_area;
	struct vm_area *prev = NULL;
	while(vm_area_head != NULL){
		if(prev != NULL){
			if(prev->vm_end == vm_area_head->vm_start && prev->access_flags == vm_area_head->access_flags){
				prev->vm_next = vm_area_head->vm_next;
				prev->vm_end = vm_area_head->vm_end;
				struct vm_area *temp = vm_area_head;
				vm_area_head = vm_area_head->vm_next;
				dealloc_vm_area(temp);
			} else {
				prev = vm_area_head;
				vm_area_head = vm_area_head->vm_next;
			}
		} else {
			prev = vm_area_head;
			vm_area_head = vm_area_head->vm_next;
		}
	}
}

void modify_physical_pages(struct exec_context *current, u64 start, u64 end, u32 access_flags){

	u64 length = end - start;
	void *base_addr = osmap(current->pgd);
	for(int i=0;i<length/PAGE_SIZE;i++){
		u64 addr = start + i * PAGE_SIZE;
		u64 *pte = get_user_pte(current, addr, 0);	
		if(!pte){
			continue;
		} else {
			if(access_flags & PROT_WRITE){
				*pte |= PROT_WRITE; 
			} else {
				if(*pte & PROT_WRITE){
					*pte ^= PROT_WRITE;
				}
			}

			asm volatile ("invlpg (%0);" 
		    :: "r"(addr) 
			: "memory");   // Flush TLB
		}

		u64 ppid = current->ppid;
		if(ppid){
			struct exec_context *parent_ctx = get_ctx_by_pid(ppid);
			if(parent_ctx->state == WAITING){
				u64 *parent_pte_entry = get_user_pte(parent_ctx, addr, 0);
				if(!parent_pte_entry) continue;;
	    		if(access_flags & PROT_WRITE){
	    			*parent_pte_entry |= PROT_WRITE;
	    		} else if(*parent_pte_entry & PROT_WRITE){
	    			*parent_pte_entry ^= PROT_WRITE;
	    		}
			}
		}
	}
}

int check_contiguous_range(struct exec_context *current, u64 addr, int length){

	struct vm_area *vm_area_head = current->vm_area;
	long start = addr;
	long end =  addr + length;
	int flag = 0;
	long prev = MMAP_AREA_START;
	while(vm_area_head != NULL){
		if(vm_area_head->vm_start >= end){
			break;
		} else if(vm_area_head->vm_end <= start){
			// do nothing
		} else {
			if(!flag){
				flag = 1;
			} else {
				if(prev != vm_area_head->vm_start){
					flag = 0;
					break;
				}
			}
		}

		prev = vm_area_head->vm_end;
		vm_area_head = vm_area_head->vm_next;
	}

	return flag;
}

int count_vm_areas(struct exec_context *current){
	struct vm_area *vm_area_head = current->vm_area;
	int cnt = 0;
	while(vm_area_head != NULL){
		cnt++;
		vm_area_head = vm_area_head->vm_next;
	}
	return cnt;
}

/**
 * mprotect System call Implementation.
 */

int vm_area_mprotect(struct exec_context *current, u64 addr, int length, int prot)
{
	struct vm_area *vm_area_head = current->vm_area;
	long start = addr;
	long end =  addr + length;

	// first check whether the given range [start, end) is valid
	if(!check_contiguous_range(current, addr, length)){
		return -1;
	}

	while(vm_area_head != NULL){
		if(vm_area_head->vm_start >= end){
			break;
		}else if(vm_area_head->vm_end <= start){
			// do nothing	
		} else if(vm_area_head->vm_start >= start && vm_area_head->vm_end <= end){
			vm_area_head->access_flags = prot;
			modify_physical_pages(current, vm_area_head->vm_start, vm_area_head->vm_end, prot);
		} else if(vm_area_head->vm_start >= start){
			if(vm_area_head->access_flags != prot){
				modify_physical_pages(current, vm_area_head->vm_start, end, prot);
				struct vm_area *new_vm_area = alloc_vm_area();
				new_vm_area->vm_start = end;
				new_vm_area->vm_end = vm_area_head->vm_end;
				new_vm_area->vm_next = vm_area_head->vm_next;
				new_vm_area->access_flags = vm_area_head->access_flags;
				vm_area_head->vm_end = end;
				vm_area_head->vm_next = new_vm_area;
				vm_area_head->access_flags = prot;
			}
		} else if(vm_area_head->vm_end <= end){
			if(vm_area_head->access_flags != prot){
				modify_physical_pages(current, start, vm_area_head->vm_end, prot);
				struct vm_area *new_vm_area = alloc_vm_area();
				new_vm_area->vm_start = start;
				new_vm_area->vm_end = vm_area_head->vm_end;
				new_vm_area->vm_next = vm_area_head->vm_next;
				new_vm_area->access_flags = prot;
				vm_area_head->vm_end = start;
				vm_area_head->vm_next = new_vm_area;
			}
		} else if(start > vm_area_head->vm_start && end < vm_area_head->vm_end){
			if(vm_area_head->access_flags != prot){
				modify_physical_pages(current, start, end, prot);
				struct vm_area *new_vm_area1 = alloc_vm_area();
				struct vm_area *new_vm_area2 = alloc_vm_area();
				new_vm_area1->vm_start = start;
				new_vm_area1->vm_end = end;
				new_vm_area1->vm_next = new_vm_area2;
				new_vm_area1->access_flags = prot;

				new_vm_area2->vm_start = end;
				new_vm_area2->vm_end = vm_area_head->vm_end;
				new_vm_area2->vm_next = vm_area_head->vm_next;
				new_vm_area2->access_flags = vm_area_head->access_flags;

				vm_area_head->vm_next = new_vm_area1;
				vm_area_head->vm_end = start;

				vm_area_head = vm_area_head->vm_next;
			}
		}

		vm_area_head = vm_area_head->vm_next;
	}
	merge_nodes(current);
    return 0;
}

/**
 * mmap system call implementation.
 */

long vm_area_without_hint(struct exec_context *current, u64 addr, int length, int prot, int flags){

	long st_addr = MMAP_AREA_START;
	struct vm_area *vm_area_head = current->vm_area;
	struct vm_area *prev = NULL;
	long start = st_addr;
	int flag = 0;
	while(vm_area_head != NULL){
		if(vm_area_head->vm_start > start + length){ // merging on left side if possible
			if(prev != NULL && prev->access_flags == prot){
				prev->vm_end = start + length; // merging on left side
			} else { // no merging
				struct vm_area *new_vm_area = alloc_vm_area();
				new_vm_area->vm_start = start;
				new_vm_area->vm_end	 = start + length;
				new_vm_area->vm_next = vm_area_head;
				new_vm_area->access_flags = prot;

				if(prev != NULL){
					prev->vm_next = new_vm_area;
				} else {
					current->vm_area = new_vm_area;
				}
			}
			flag = 1;
		} else if(vm_area_head->vm_start == start + length){
			if(prev != NULL && prev->access_flags == prot){
				if(vm_area_head->access_flags == prot){ // merging on both sides
					prev->vm_end = vm_area_head->vm_end;
					prev->vm_next = vm_area_head->vm_next;
					struct vm_area *temp = vm_area_head;
					vm_area_head = prev;
					dealloc_vm_area(temp);
				} else { // merging on left side
					prev->vm_end = start + length;
				}
			} else {
				if(vm_area_head->access_flags == prot){ // merging on the right side
					vm_area_head->vm_start = start;
				} else { // no merging
					struct vm_area *new_vm_area = alloc_vm_area();
					new_vm_area->vm_start = start;
					new_vm_area->vm_end	 = start + length;
					new_vm_area->vm_next = vm_area_head;
					new_vm_area->access_flags = prot;

					if(prev != NULL){
						prev->vm_next = new_vm_area;
					} else {
						current->vm_area = new_vm_area;
					}
				}
			}
			flag = 1;
		}

		if(flag){
			break;
		}

		start = vm_area_head->vm_end;
		prev = vm_area_head;
		vm_area_head = vm_area_head->vm_next;
	}

	if(!flag){
		if(prev->access_flags == prot){
			prev->vm_end = start + length;
		} else {
			struct vm_area *new_vm_area = alloc_vm_area();
			new_vm_area->vm_start = start;
			new_vm_area->vm_end = start + length;
			new_vm_area->vm_next = NULL;
			new_vm_area->access_flags = prot;
			prev->vm_next = new_vm_area;
		}
	}
	vm_area_head = current->vm_area;
	while(vm_area_head!=NULL){
		vm_area_head = vm_area_head->vm_next;
	}

	return start; 
}


long vm_area_with_hint(struct exec_context *current, u64 addr, int length, int prot, int flags){

	long start = addr;
	int flag = 0;
	struct vm_area *prev = NULL;
	struct vm_area *vm_area_head = current->vm_area;

	while(vm_area_head != NULL){
		if(vm_area_head->vm_start > start + length &&
		 (prev == NULL || prev->vm_end <= start)){ // merging on the left side if possible
			if(prev != NULL && prev->vm_end == start && prev->access_flags == prot){
				prev->vm_end = start + length; // merging on the left side;
			} else { // no merging;
				struct vm_area *new_vm_area = alloc_vm_area();
				new_vm_area->vm_start = start;
				new_vm_area->vm_end = start + length;
				new_vm_area->vm_next = vm_area_head;
				new_vm_area->access_flags = prot;

				if(prev != NULL){
					prev->vm_next = new_vm_area;
				} else {
					current->vm_area = new_vm_area;
				}
			}
			flag = 1;
		} else if(vm_area_head->vm_start == start + length 
			&& (prev == NULL || prev->vm_end <= start)){
			if(prev != NULL && prev->vm_end == start && prev->access_flags == prot){ 
				if(vm_area_head->access_flags == prot){ // merging on both sides
					prev->vm_end = vm_area_head->vm_end;
					prev->vm_next = vm_area_head->vm_next;
					struct vm_area *temp = vm_area_head;
					vm_area_head = prev;
					dealloc_vm_area(temp);
				} else { // merging on left side
					prev->vm_end = start + length;
				}
			} else {
				if(vm_area_head->access_flags == prot){ // merging on right side
					vm_area_head->vm_start = start;
				} else { // no merging
					struct vm_area *new_vm_area = alloc_vm_area();
					new_vm_area->vm_start = start;
					new_vm_area->vm_end	 = start + length;
					new_vm_area->vm_next = vm_area_head;
					new_vm_area->access_flags = prot;

					if(prev != NULL){
						prev->vm_next = new_vm_area;
					} else {
						current->vm_area = new_vm_area;
					}
				}
			}
			flag = 1;
		}

		if(flag){
			break;
		}

		prev = vm_area_head;
		vm_area_head = vm_area_head->vm_next;
	}

	if(!flag){
		if(prev->vm_end == start && prev->access_flags == prot){
			prev->vm_end = start + length;
			flag = 1;
		} else if(prev->vm_end <= start) {
			struct vm_area *new_vm_area = alloc_vm_area();
			new_vm_area->vm_start = start;
			new_vm_area->vm_end = start + length;
			new_vm_area->vm_next = NULL;
			new_vm_area->access_flags = prot;
			prev->vm_next = new_vm_area;
			flag = 1;
		}
	}

	return flag;
}


long vm_area_map(struct exec_context *current, u64 addr, int length, int prot, int flags)
{	
	long returnAddr = -1;
	struct vm_area *vm_area_head = current->vm_area;
	long st_addr = MMAP_AREA_START;

	// page-align length
	if(length % PAGE_SIZE != 0){
		length = (length/PAGE_SIZE + 1) * PAGE_SIZE;
	}
	// if head is null
	if(vm_area_head == NULL){
		struct vm_area *new_vm_area = alloc_vm_area();
		if(addr == 0)addr = st_addr;
		new_vm_area->vm_start = addr;
		new_vm_area->vm_end = addr + length;
		new_vm_area->vm_next = NULL;
		new_vm_area->access_flags = prot;
		current->vm_area = new_vm_area;
		returnAddr = addr;
	} else {
		// if hint is not given
		if(addr == 0){
			returnAddr =  vm_area_without_hint(current,addr,length,prot,flags);
		} else {
			// if hint is given
			long ret = vm_area_with_hint(current,addr,length,prot,flags);
			if(ret == 1){
				returnAddr = addr;
			} else if(!(flags & MAP_FIXED)){
				returnAddr = vm_area_without_hint(current,addr,length,prot,flags);
			} else {
				returnAddr = -1;
			}
		}
	}

	if(returnAddr != -1 && count_vm_areas(current) > 128){
		vm_area_unmap(current, returnAddr, length);
		return -1;
	}

	if(returnAddr == -1 || !(flags & MAP_POPULATE)){
		u64 ppid = current->ppid;
		if(ppid){
			struct exec_context *parent_ctx = get_ctx_by_pid(ppid);
			if(parent_ctx->state == WAITING){
				parent_ctx->vm_area = current->vm_area;
			}
		}
		return returnAddr;
	}
	else {
		void *base_addr;
		base_addr = osmap(current->pgd);
		for(int i=0;i<length/PAGE_SIZE;i++){
			u64 addr1 = addr + i * PAGE_SIZE;
			map_physical_page((u64) base_addr, addr1, prot, 0);
		}

		u64 ppid = current->ppid;
		if(ppid){
			struct exec_context *parent_ctx = get_ctx_by_pid(ppid);
			if(parent_ctx->state == WAITING){
				parent_ctx->vm_area = current->vm_area;
			}
		}

		return returnAddr;
	}
}

void unmap_physical_pages(struct exec_context *current, u64 start, u64 end, u32 access_flags){

	u64 length = end - start;

	for(int i=0;i<length/PAGE_SIZE;i++){
		u64 addr = start + i * PAGE_SIZE;
	    u64 *pte_entry = get_user_pte(current, addr, 0);
	    if(!pte_entry)
	        continue;
	  
	    os_pfn_free(USER_REG, (*pte_entry >> PTE_SHIFT) & 0xFFFFFFFF);
	    *pte_entry = 0;  // Clear the PTE
	  
	    asm volatile ("invlpg (%0);" 
	                    :: "r"(addr) 
	                    : "memory");   // Flush TLB
	    
	    u64 ppid = current->ppid;
		if(ppid){
			struct exec_context *parent_ctx = get_ctx_by_pid(ppid);
			if(parent_ctx->state == WAITING){
				u64 *parent_pte_entry = get_user_pte(parent_ctx, addr, 0);
				if(!parent_pte_entry) continue;
				os_pfn_free(USER_REG, (*parent_pte_entry >> PTE_SHIFT) & 0xFFFFFFFF);
	    		*parent_pte_entry = 0;  // Clear the PTE
			}
		}
	}
}

/**
 * munmap system call implemenations
 */

int vm_area_unmap(struct exec_context *current, u64 addr, int length)
{
	
	struct vm_area *vm_area_head = current->vm_area;
	// page-align length
	if(length % PAGE_SIZE != 0){
		length = (length/PAGE_SIZE + 1) * PAGE_SIZE;
	}

	long start = addr;
	long end =  addr + length; // the interval [start, end) is deleted
	long rel2 = (end - MMAP_AREA_START)/PAGE_SIZE;
	long rel1 = (start - MMAP_AREA_START)/PAGE_SIZE;
	struct vm_area *prev = NULL;
	while(vm_area_head != NULL){
		if(vm_area_head->vm_start >= end){
			break;
		}
		else if(vm_area_head->vm_end <= start){
			// do nothing
		} else if(vm_area_head->vm_start >= start && vm_area_head->vm_end <= end){
			if(prev != NULL){
				prev->vm_next = vm_area_head->vm_next;
			} else {
				current->vm_area = vm_area_head->vm_next;
			}
			struct vm_area *temp1 = vm_area_head;
			unmap_physical_pages(current, vm_area_head->vm_start, vm_area_head->vm_end, vm_area_head->access_flags);
			vm_area_head = vm_area_head->vm_next;
			dealloc_vm_area(temp1);
			continue;
		} else if(vm_area_head->vm_start >= start){
			unmap_physical_pages(current, vm_area_head->vm_start, end, vm_area_head->access_flags);
			vm_area_head->vm_start = end;
		} else if(vm_area_head->vm_end <= end){
			unmap_physical_pages(current, start, vm_area_head->vm_end, vm_area_head->access_flags);
			vm_area_head->vm_end = start; 
		} else if(start > vm_area_head->vm_start && end < vm_area_head->vm_end){
			unmap_physical_pages(current, start, end, vm_area_head->access_flags);
			struct vm_area *new_vm_area = alloc_vm_area();
			new_vm_area->vm_start = end;
			new_vm_area->vm_end = vm_area_head->vm_end;
			new_vm_area->vm_next = vm_area_head->vm_next;
			new_vm_area->access_flags = vm_area_head->access_flags;
			vm_area_head->vm_next = new_vm_area;
			vm_area_head->vm_end = start;
		}
		prev = vm_area_head;
		vm_area_head = vm_area_head->vm_next;
	}

	u64 ppid = current->ppid;
	if(ppid){
		struct exec_context *parent_ctx = get_ctx_by_pid(ppid);
		if(parent_ctx->state == WAITING){
			parent_ctx->vm_area = current->vm_area;
		}
	}

    int isValid = 0;
    return isValid;
}