// Aman Tiwari , Roll No - 160094

#include <cfork.h>
#include <page.h>
#include <mmap.h>

/* You need to implement cfork_copy_mm which will be called from do_cfork in entry.c. Don't remove copy_os_pts()*/
void cfork_copy_mm(struct exec_context *child, struct exec_context *parent ){

   void *os_addr;
   u64 vaddr; 
   struct mm_segment *seg;

   child->pgd = os_pfn_alloc(OS_PT_REG);

   os_addr = osmap(child->pgd);
   bzero((char *)os_addr, PAGE_SIZE);
   
   //CODE segment
   seg = &parent->mms[MM_SEG_CODE];
   for(vaddr = seg->start; vaddr < seg->next_free; vaddr += PAGE_SIZE){
      u64 *parent_pte =  get_user_pte(parent, vaddr, 0);
      if(parent_pte){
        u64 pfn = install_ptable((u64) os_addr, seg, vaddr, (*parent_pte & FLAG_MASK) >> PAGE_SHIFT);
        struct pfn_info *p = get_pfn_info(pfn);
        increment_pfn_info_refcount(p); 
      }  
   } 

   //RODATA segment
   seg = &parent->mms[MM_SEG_RODATA];
   for(vaddr = seg->start; vaddr < seg->next_free; vaddr += PAGE_SIZE){
      u64 *parent_pte =  get_user_pte(parent, vaddr, 0);
      if(parent_pte){
        u64 pfn = install_ptable((u64)os_addr, seg, vaddr, (*parent_pte & FLAG_MASK) >> PAGE_SHIFT);
        struct pfn_info *p = get_pfn_info(pfn);
        increment_pfn_info_refcount(p);
       }   
   } 
   
   //DATA segment
  seg = &parent->mms[MM_SEG_DATA];
  for(vaddr = seg->start; vaddr < seg->next_free; vaddr += PAGE_SIZE){
      u64 *parent_pte =  get_user_pte(parent, vaddr, 0);
      if(parent_pte){
      	    *parent_pte ^= PROT_WRITE;
      	    seg->access_flags = PROT_READ;
            u64 pfn = install_ptable((u64)os_addr, seg, vaddr, (*parent_pte & FLAG_MASK) >> PAGE_SHIFT);
            struct pfn_info *p = get_pfn_info(pfn);
        	increment_pfn_info_refcount(p);
            seg->access_flags = PROT_WRITE | PROT_READ;  
      }
  } 
  
  //STACK segment
  seg = &parent->mms[MM_SEG_STACK];
  for(vaddr = seg->end - PAGE_SIZE; vaddr >= seg->next_free; vaddr -= PAGE_SIZE){
      u64 *parent_pte = get_user_pte(parent, vaddr, 0);
     if(parent_pte){
           u64 pfn = install_ptable((u64)os_addr, seg, vaddr, 0);  //Returns the blank page
           pfn = (u64)osmap(pfn);
           memcpy((char *)pfn, (char *)(*parent_pte & FLAG_MASK), PAGE_SIZE); 
      }
  }

  struct vm_area *vm_area_head = parent->vm_area;
  struct vm_area *prev = NULL;

  while(vm_area_head != NULL){
  	u64 start = vm_area_head->vm_start;
  	u64 end = vm_area_head->vm_end;
  	u64 length = (end - start)/PAGE_SIZE;
  	struct vm_area *new_vm_area = alloc_vm_area();

  	if(prev == NULL){
  		prev = new_vm_area;
  		child->vm_area = prev;
  	} else {
  		prev->vm_next = new_vm_area;
  	}

  	new_vm_area->vm_start = vm_area_head->vm_start;
  	new_vm_area->vm_end = vm_area_head->vm_end;
  	new_vm_area->vm_next = NULL;
  	new_vm_area->access_flags = vm_area_head->access_flags;

  	for(int i=0;i<length;i++){
  		u64 addr = start + i * PAGE_SIZE;
  		u64 *parent_pte = get_user_pte(parent, addr, 0);
  		u32 access_flags = PROT_READ;
  		if(parent_pte){
  			if(*parent_pte & PROT_WRITE)*parent_pte ^= PROT_WRITE;
  			u64 pfn = map_physical_page((u64)os_addr, addr, access_flags, (*parent_pte & FLAG_MASK) >> PAGE_SHIFT);
  			struct pfn_info *p = get_pfn_info(pfn);
        	increment_pfn_info_refcount(p);
  		}
  	}
  	
  	prev = new_vm_area;
  	vm_area_head = vm_area_head->vm_next;

  }

  copy_os_pts(parent->pgd, child->pgd);
  return;
    
}

/* You need to implement cfork_copy_mm which will be called from do_vfork in entry.c.*/
void vfork_copy_mm(struct exec_context *child, struct exec_context *parent ){
  	
	void *os_addr;
	u64 vaddr; 
	struct mm_segment *seg;

	child->pgd = os_pfn_alloc(OS_PT_REG);

	os_addr = osmap(child->pgd);
	bzero((char *)os_addr, PAGE_SIZE);

	//CODE segment
	seg = &parent->mms[MM_SEG_CODE];
	for(vaddr = seg->start; vaddr < seg->next_free; vaddr += PAGE_SIZE){
	  u64 *parent_pte =  get_user_pte(parent, vaddr, 0);
	  if(parent_pte){
	    install_ptable((u64) os_addr, seg, vaddr, (*parent_pte & FLAG_MASK) >> PAGE_SHIFT);
	  }
	} 		

	//RODATA segment
	seg = &parent->mms[MM_SEG_RODATA];
	for(vaddr = seg->start; vaddr < seg->next_free; vaddr += PAGE_SIZE){
	  u64 *parent_pte =  get_user_pte(parent, vaddr, 0);
	  if(parent_pte)
	    install_ptable((u64)os_addr, seg, vaddr, (*parent_pte & FLAG_MASK) >> PAGE_SHIFT);   
	} 

	//DATA segment
	seg = &parent->mms[MM_SEG_DATA];
	for(vaddr = seg->start; vaddr < seg->next_free; vaddr += PAGE_SIZE){
	  u64 *parent_pte =  get_user_pte(parent, vaddr, 0);
	  if(parent_pte){
	       install_ptable((u64)os_addr, seg, vaddr, (*parent_pte & FLAG_MASK) >> PAGE_SHIFT);
	  }
	} 

	//STACK segment
	seg = &parent->mms[MM_SEG_STACK];
	for(vaddr = seg->end - PAGE_SIZE; vaddr >= seg->next_free; vaddr -= PAGE_SIZE){
	u64 *parent_pte = get_user_pte(parent, vaddr, 0);
	 if(parent_pte){
           u64 pfn = install_ptable((u64)os_addr, seg, vaddr, 0);  //Returns the blank page  
           pfn = (u64)osmap(pfn);
           memcpy((char *)pfn, (char *)(*parent_pte & FLAG_MASK), PAGE_SIZE);
        }
	}

	struct vm_area *vm_area_head = parent->vm_area;

	while(vm_area_head != NULL){
		u64 start = vm_area_head->vm_start;
		u64 end = vm_area_head->vm_end;
		u64 length = (end - start)/PAGE_SIZE;

		for(int i=0;i<length;i++){
			u64 addr = start + i * PAGE_SIZE;
			u64 *parent_pte = get_user_pte(parent, addr, 0);
			if(parent_pte){
				map_physical_page((u64)os_addr, addr, vm_area_head->access_flags, (*parent_pte & FLAG_MASK) >> PAGE_SHIFT);
			}
		}

		vm_area_head = vm_area_head->vm_next;
	}

	parent->state = WAITING;

	copy_os_pts(parent->pgd, child->pgd);	
    return;
    
}

/*You need to implement handle_cow_fault which will be called from do_page_fault 
incase of a copy-on-write fault

* For valid acess. Map the physical page 
 * Return 1
 * 
 * For invalid access,
 * Return -1. 
*/

int handle_cow_fault(struct exec_context *current, u64 cr2){

	void *os_addr;
	os_addr = osmap(current->pgd);
	struct mm_segment *seg;
	seg = &current->mms[MM_SEG_DATA];
	u64 start = seg->start;
	u64 end = seg->end;

	u64 *pte = get_user_pte(current, cr2, 0);
	u64 pfn =  (*pte & FLAG_MASK) >> PAGE_SHIFT;
	struct pfn_info *p = get_pfn_info(pfn);
	u64 ref_count = get_pfn_info_refcount(p);

	if(cr2 >= start && cr2 < end){
	   if(ref_count > 1){
		   u64 pfn1 = install_ptable((u64)os_addr, seg, cr2, 0);  //Returns the blank page
		   decrement_pfn_info_refcount(p);  
	       pfn1 = (u64)osmap(pfn1);
	       memcpy((char *)pfn1, (char *)(*pte & FLAG_MASK), PAGE_SIZE);
	       return 1;
	   } else {
	   		if(seg->access_flags & PROT_WRITE){
	   			*pte = *pte | PROT_WRITE;
	   			return 1;
	   		} else {
	   			return -1;
	   		}
	   }
    }

	struct vm_area *vm_area_head = current->vm_area;
	int flag = 0;
	while(vm_area_head !=  NULL){
		if(vm_area_head->vm_start <= cr2 && vm_area_head->vm_end > cr2){
			flag = 1;
			break;
		}
		vm_area_head = vm_area_head->vm_next;
	}

	if(flag){
		if(vm_area_head->access_flags & PROT_WRITE){
			if(ref_count > 1){
				u64 pfn1 = map_physical_page((u64)os_addr, cr2, PROT_WRITE, 0);
				decrement_pfn_info_refcount(p);
				pfn1 = (u64)osmap(pfn1);
	       		memcpy((char *)pfn1, (char *)(*pte & FLAG_MASK), PAGE_SIZE);
				return 1;
			} else {
				*pte = *pte | PROT_WRITE;
				return 1;
			}
		} else {
			return -1;
		}
	} else {
		return -1;
	}
}

/* You need to handle any specific exit case for vfork here, called from do_exit*/
void vfork_exit_handle(struct exec_context *ctx){
	
	u64 parent_pid = ctx->ppid;
	if(parent_pid){
	    struct exec_context *parent_ctx = get_ctx_by_pid(parent_pid);
	    if(parent_ctx->state == WAITING)
	   		parent_ctx->state = READY;
	}

	return;
}