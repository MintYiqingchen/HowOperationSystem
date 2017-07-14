#include "osmain.h"
int memman_alloc_4k(struct MEMMAN* man,unsigned int size){
	int a;
	size=(size+0xfff)&0xfffff000;//向上取整
	a=memman_alloc(man,size);
	return a;
}

int memman_free_4k(struct MEMMAN*man,unsigned int addr,unsigned int size){
	int i;
	size=(size+0xfff)&0xfffff000;
	i=memman_free(man,addr,size);
	return i;
}
void memtest(struct MEMMAN* man){
	unsigned int count=0,addr=MemChkBuf,i,size=0;
	//获得ARD块数和存储起始地址
	count=*((unsigned int*)dwMCRNumber);
	for(i=0;i<count;i++){
		struct AddrRangeDesc* ard=(struct AddrRangeDesc*)addr;
		if(ard->type==1){
			unsigned int baddr=ard->baseAddrLow;
			size=ard->lengthLow;
			memman_free(man,baddr,size);
		}
		addr=addr+20;//指向下一个ard
	}
	return ;
}
void memman_init(struct MEMMAN* man){
	man->frees=0;
	man->maxfrees=0;
	man->lostsize=0;
	man->losts=0;
}

unsigned int memman_total(struct MEMMAN* man){
	unsigned int res=0;
	unsigned int i;
	for(i=0;i<man->frees;++i){
		res+=man->free[i].size;
	}
	return res;
}

int memman_alloc(struct MEMMAN* man,unsigned int size){
	int i,addr=-1;//-1 代表无可用内存
	for(i=0;i<man->frees;++i){
		if(man->free[i].size>=size){
			addr=man->free[i].addr;
			man->free[i].addr+=size;
			man->free[i].size-=size;
			if(man->free[i].size==0){
				man->frees--;
			}
			break;
		}
	}
	return addr;
}

int memman_free(struct MEMMAN* man,unsigned int addr,unsigned int size){
	unsigned int i;
	for(i=0;i<man->frees;++i){
		if(man->free[i].addr>addr)
			break;
	}//i值是释放地址应该位于free表中的位置

	if(i>0){
		/*与前方内存归并*/
		if(man->free[i-1].addr+man->free[i-1].size>=addr){
			man->free[i-1].size+=size;
			/*与后方归并*/
			if(addr+size>=man->free[i].addr){
				man->free[i-1].size+=man->free[i].size;
				man->frees--;
				for(;i<man->frees;++i){
					man->free[i]=man->free[i+1];
				}
			}
			return 0;
		}
	}
	if(i<man->frees){
		/*与后方归并*/
		if(addr+size>=man->free[i].addr){
			man->free[i].addr=addr;
			man->free[i].size+=size;
			return 0;
		}
	}

	//不能归并，但还有剩余的管理空间
	if(man->frees<MEMMAN_FREES){
		int j;
		for (j=man->frees;j>i;--j){
			man->free[j]=man->free[j-1];
		}
		man->free[i].addr=addr;
		man->free[i].size=size;
		man->frees++;
		if(man->maxfrees<man->frees){
			man->maxfrees=man->frees;
		}
		return 0;
	}
	//无法存下释放内存信息
	man->losts++;
	man->lostsize+=size;
	return -1;
}