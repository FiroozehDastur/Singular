/*******************************************************************
 *  File:    omDebugCheck.c
 *  Purpose: implementation of omCheck functions
 *  Author:  obachman@mathematik.uni-kl.de (Olaf Bachmann)
 *  Created: 11/99
 *  Version: $Id: omDebugCheck.c,v 1.5 2000-09-14 12:59:52 obachman Exp $
 *******************************************************************/
#include <limits.h>
#include <stdarg.h>

#include "omAlloc.h"
#include "omDebug.h"

#ifndef OM_NDEBUG
/*******************************************************************
 *  
 *   Declarations: Static function
 *  
 *******************************************************************/
unsigned long om_MaxAddr = 0;
unsigned long om_MinAddr = ULONG_MAX;
static omError_t omDoCheckLargeAddr(void* addr, void* bin_size, omTrackFlags_t flags, char level,
                                    omError_t report, OM_FLR_DECL);
omError_t omDoCheckBin(omBin bin, int normal_bin, char level, 
                       omError_t report, OM_FLR_DECL);
static omError_t omDoCheckBinPage(omBinPage page, int normal_page, int level, 
                                  omError_t report, OM_FLR_DECL);
static void _omPrintAddrInfo(FILE* fd, omError_t error, void* addr, void* bin_size, omTrackFlags_t flags, char* s);


/*******************************************************************
 *  
 * First level omCheck routines: dispatch to lower-level omDoCheck 
 * routines which do the actual tests
 *  
 *******************************************************************/
omError_t _omCheckAddr(void* addr, void* size_bin, 
                       omTrackFlags_t flags, char check, omError_t report, OM_FLR_DECL)
{
  if (check <= 0) return omError_NoError;
  if (check > 1)
  {
    omCheckReturn(check > 2 && _omCheckMemory(check - 2, (report ? report : omError_MemoryCorrupted), OM_FLR_VAL));
    omCheckReturn(omIsBinPageAddr(addr) && omDoCheckBin(omGetBinOfAddr(addr), !omIsBinAddrTrackAddr(addr), check-1, 
                                                        (report ? report : omError_MemoryCorrupted), OM_FLR_VAL));
  }
  return omDoCheckAddr(addr, size_bin, flags, check, report, OM_FLR_VAL);
}

omError_t _omCheckBin(omBin bin, int what_bin, char check, omError_t report, OM_FLR_DECL)
{
  if (check <= 0) return omError_NoError;

  omCheckReturn(check > 1 && _omCheckMemory(check - 1, (report ? report : omError_MemoryCorrupted), OM_FLR_VAL));

  return omDoCheckBin(bin, what_bin, check, report, OM_FLR_VAL);
}

omError_t _omCheckMemory(char check, omError_t report, OM_FLR_DECL)
{
  int i = 0;
  omSpecBin s_bin;
  
  if (check <= 0) return omError_NoError;
  
  omCheckReturn(omCheckBinPageRegions(check, report, OM_FLR_VAL));
  
  for (i=0; i<= OM_MAX_BIN_INDEX; i++)
  {
    omCheckReturn(omDoCheckBin(&om_StaticBin[i], 1, check, report, OM_FLR_VAL));
  }

  s_bin = om_SpecBin;
  omCheckReturn(omCheckGList(s_bin, next, check, omError_MemoryCorrupted, OM_FLR_VAL));
  while (s_bin != NULL)
  {
    omCheckReturn(omDoCheckBin(s_bin->bin, 1, check, report, OM_FLR_VAL));
    s_bin = s_bin->next;
  }

#ifdef OM_HAVE_TRACK
  for (i=0; i<= OM_MAX_BIN_INDEX; i++)
  {
    omCheckReturn(omDoCheckBin(&om_StaticTrackBin[i], 0, check, report, OM_FLR_VAL));
  }
  s_bin = om_SpecTrackBin;
  omCheckReturn(omCheckGList(s_bin, next, check, omError_MemoryCorrupted, OM_FLR_VAL));
  while (s_bin != NULL)
  {
    omCheckReturn(omDoCheckBin(s_bin->bin, 0, check, report, OM_FLR_VAL));
    s_bin = s_bin->next;
  }
#endif
  
  if (check > 1 && om_KeptAddr != NULL)
  {
    void* addr = om_KeptAddr;
    omCheckReturn(omCheckList(om_KeptAddr, check, (report ? report :  omError_KeptAddrListCorrupted), OM_FLR_VAL));
    while (addr != NULL)
    {
      omCheckReturn(omDoCheckAddr(addr, NULL, OM_FKEPT, check, report, OM_FLR_VAL));
      addr = *((void**) addr);
    }
  }
  return omError_NoError;
}

/*******************************************************************
 *  
 * Second level omCheck routines: do the actual checks
 *  
 *******************************************************************/

omError_t omCheckPtr(void* ptr, omError_t report, OM_FLR_DECL)
{
  omCheckReturnError(ptr == NULL, omError_NullAddr);
  omCheckReturnError(!OM_IS_ALIGNED(ptr), omError_UnalignedAddr);
  omCheckReturnError(((unsigned long) ptr) < om_MinAddr || 
                     ((unsigned long) ptr) >= om_MaxAddr, omError_InvalidRangeAddr);
  return omError_NoError;
}
  

omError_t omDoCheckAddr(void* addr, void* bin_size, omTrackFlags_t flags, char level, 
                        omError_t report, OM_FLR_DECL)
{
  omAssume(! ((flags & OM_FSIZE) && (flags & OM_FBIN)));

  if (addr == NULL)
  {
    omCheckReturnError(!(flags & OM_FSLOPPY), omError_NullAddr);
    return omError_NoError;
  }
  if ((flags & OM_FSIZE) && bin_size == NULL) return omError_NoError;
  omAddrCheckReturn(omCheckPtr(addr, report, OM_FLR_VAL));
  omAddrCheckReturnError((flags & OM_FALIGN) &&  !OM_IS_STRICT_ALIGNED(addr), omError_UnalignedAddr);
  omAddrCheckReturnError((flags & OM_FBIN) && !omIsKnownTopBin((omBin) bin_size, 1), omError_UnknownBin);
  
  if (omIsBinPageAddr(addr))
  {
#ifdef OM_HAVE_TRACK
    if (omIsBinAddrTrackAddr(addr))
      return omCheckTrackAddr(addr, bin_size, flags, level, report, OM_FLR_VAL);
    else
#endif
      return omDoCheckBinAddr(addr, bin_size, flags, level, report, OM_FLR_VAL);
  }
  else
  {
    return omDoCheckLargeAddr(addr, bin_size, flags, level, report, OM_FLR_VAL);
  }
}

static int omIsInKeptAddrList(void* addr)
{
  void* ptr = om_KeptAddr;
  
  while (ptr != NULL)
  {
    if (ptr == addr) return 1;
    if ((unsigned long) addr > (unsigned long)ptr && 
        (unsigned long) addr < (unsigned long) ptr + omSizeOfAddr(ptr))
      return 1;
    ptr = *((void**) ptr);
  }
  return 0;
}


static omError_t omDoCheckLargeAddr(void* addr, void* bin_size, omTrackFlags_t flags, char level, 
                                    omError_t report, OM_FLR_DECL)
{
  size_t r_size;
  
  omAssume(! omIsBinPageAddr(addr));
  omAssume(! omCheckPtr(addr, omError_NoError, OM_FLR));
  
  omAddrCheckReturnError((flags & OM_FBIN) || (flags & OM_FBINADDR), omError_NotBinAddr);
  omAddrCheckReturnError(level > 1 && omFindRegionOfAddr(addr) != NULL, omError_FreedAddrOrMemoryCorrupted);
  r_size = omSizeOfLargeAddr(addr);
  omAddrCheckReturnError(! OM_IS_ALIGNED(r_size), omError_FalseAddrOrMemoryCorrupted);
  omAddrCheckReturnError(r_size <= OM_MAX_BLOCK_SIZE, omError_FalseAddrOrMemoryCorrupted);
  omAddrCheckReturnError((flags & OM_FSIZE) && r_size < OM_ALIGN_SIZE((size_t) bin_size),
                         omError_WrongSize);
  omAddrCheckReturnError((level > 1) && (flags & OM_FUSED) && omIsInKeptAddrList(addr), omError_FreedAddr);
  return omError_NoError;
}

omError_t omDoCheckBinAddr(void* addr, void* bin_size, omTrackFlags_t flags, char level, 
                           omError_t report, OM_FLR_DECL)
{
  omBinPage page = omGetBinPageOfAddr(addr);
  omBinPageRegion region = page->region;
  omBin bin = omGetBinOfPage(page);
  
  omAssume(omIsBinPageAddr(addr));
  omAssume(! omCheckPtr(addr, 0, OM_FLR));
  
  omAddrCheckReturnCorrupted(! omIsKnownTopBin(bin, ! omIsBinAddrTrackAddr(addr)));

  if (flags & OM_FBINADDR && flags & OM_FSIZE)
    omAddrCheckReturnError(bin->sizeW*SIZEOF_LONG != (size_t) bin_size, omError_WrongSize);
    
  if (level > 1)
  {
    omAddrCheckReturnError(omIsAddrOnFreeBinPage(addr), omError_FreedAddr);
    omAddrCheckReturnError(omFindRegionOfAddr(addr) != region, omError_FreedAddrOrMemoryCorrupted);
    omAddrCheckReturnError(!omIsOnGList(bin->last_page, prev, page), omError_FreedAddrOrMemoryCorrupted);

    if (flags & OM_FUSED)
    {
      omAddrCheckReturnError(omIsOnList(page->current, addr), omError_FreedAddr);
      omAddrCheckReturnError((level > 1) && omIsInKeptAddrList(addr), omError_FreedAddr);
    }
  }
  else
  {
    omAddrCheckReturnError(omCheckPtr(region, omError_MaxError, OM_FLR_VAL), omError_FreedAddrOrMemoryCorrupted);
  }
  
  
  /* Check that addr is aligned within page of bin */
  omAddrCheckReturnError((bin->max_blocks >= 1) &&
                         ( ( ( (unsigned long) addr) 
                             - ((unsigned long) page) 
                             - SIZEOF_OM_BIN_PAGE_HEADER) 
                           % (bin->sizeW * SIZEOF_VOIDP)
                           != 0), omError_FalseAddr);

  /* Check that specified bin or size is correct */
  omAddrCheckReturnError((flags & OM_FBIN) &&  bin_size != NULL 
                         && ((omBin) bin_size) != omGetTopBinOfAddr(addr), omError_WrongBin);

  if (flags & OM_FSIZE && (!(flags & OM_FSLOPPY)  || (size_t) bin_size > 0))
  {
    size_t size = (size_t) bin_size;
    omAssume(!omIsBinAddrTrackAddr(addr));
    omAddrCheckReturnError((bin->sizeW << LOG_SIZEOF_LONG) < OM_ALIGN_SIZE(size), omError_WrongSize);
  }

  return omError_NoError;
}

omError_t omDoCheckBin(omBin bin, int normal_bin, char level, 
                       omError_t report, OM_FLR_DECL)
{
  omBin top_bin = bin;
  
  omCheckReturnError(!omIsKnownTopBin(bin, normal_bin), omError_UnknownBin);
  omCheckReturn(omCheckGList(bin->next, next, level, report, OM_FLR_VAL));
  
  do
  {
    int where;
    omBinPage page;

    if (bin->last_page == NULL || bin->current_page == om_ZeroPage)
    {
      omCheckReturnCorrupted(! (bin->current_page == om_ZeroPage && bin->last_page == NULL));
      continue;
    }
    omCheckReturn(omDoCheckBinPage(bin->current_page, normal_bin, level, report, OM_FLR_VAL));
    omCheckReturn(bin->current_page != bin->last_page  && 
                  omDoCheckBinPage(bin->last_page, normal_bin, level, report, OM_FLR_VAL));
    omCheckReturnCorrupted(bin->last_page->next != NULL);

    if (bin != top_bin)
    {
      omCheckReturnCorrupted(bin->sizeW != top_bin->sizeW ||
                             bin->max_blocks != top_bin->max_blocks);
    }
    if (level <= 1) continue;
    
    omCheckReturnCorrupted(omFindInGList(bin->next, next, sticky, bin->sticky));
    omCheckReturn(omCheckGList(bin->last_page, prev, level-1, report, OM_FLR_VAL));
    page = omGListLast(bin->last_page, prev);
    omCheckReturn(omCheckGList(page, next, level-1, report, OM_FLR_VAL));
    omCheckReturnCorrupted(omGListLength(bin->last_page, prev) != omGListLength(page, next));
    
    omCheckReturnCorrupted(! omIsOnGList(bin->last_page, prev, bin->current_page));
    
    page = bin->last_page;
    where = 1;
    while (page != NULL)
    {
      omCheckReturnCorrupted(omGetTopBinOfPage(page) != top_bin);
      omCheckReturn(page != bin->last_page && page != bin->current_page &&
                    omDoCheckBinPage(page, normal_bin, level - 1, report, OM_FLR_VAL));

      omCheckReturnCorrupted(page != bin->last_page && 
                             (page->next == NULL || page->next->prev != page));
      omCheckReturnCorrupted(page->prev != NULL && page->prev->next != page);
      
      omCheckReturnCorrupted(omGetStickyOfPage(page) != bin->sticky);
      omCheckReturnCorrupted(omGetBinOfPage(page) != bin);
      
      if (! bin->sticky)
      {
        if (where == -1)
        {
          /* we are at the left of current_page,
             i.e., page is empty */
          omCheckReturnCorrupted(omGetUsedBlocksOfPage(page) != 0 || page->current != NULL);
        }
        else
        {
          if (page == bin->current_page)
          {
            where = -1;
          }
          else
          {
            /* we are at the right of current_page, 
               i.e., page is neither full nor empty */
            omCheckReturnCorrupted(page->current == NULL ||
                                   omGetUsedBlocksOfPage(page) == bin->max_blocks - 1);
          }
        }
      }
      page = page->prev;
    }   /* while (page != NULL) */
  } while ((bin = bin->next) != NULL);
  
  return omError_NoError;
}
  
int omIsKnownTopBin(omBin bin, int normal_bin)
{
  omBin to_check;
  omSpecBin s_bin;
  int i;
  
  omAssume(normal_bin == 1 || normal_bin == 0);
  
#ifdef OM_HAVE_TRACK
  if (! normal_bin) 
  {
    to_check = om_StaticTrackBin;
    s_bin = om_SpecTrackBin;
  }
  else 
#endif
  {
    omAssume(normal_bin);
    to_check = om_StaticBin;
    s_bin = om_SpecBin;
  }

  for (i=0; i<= OM_MAX_BIN_INDEX; i++)
  {
    if (bin == &(to_check[i])) 
      return 1;
  }
  
  while (s_bin != NULL)
  {
    if (bin == s_bin->bin) return 1;
    s_bin = s_bin->next;
  }
  return 0;
}

static omError_t omDoCheckBinPage(omBinPage page, int normal_page, int level, 
                                  omError_t report, OM_FLR_DECL)
{
  omBin bin;
  
  omCheckReturn(omCheckPtr(page, report, OM_FLR_VAL));
  omCheckReturnCorrupted(! omIsAddrPageAligned(page));

  omCheckReturn(omCheckPtr(page->region, report, OM_FLR_VAL));
  omCheckReturnCorrupted(level > 1 && omFindRegionOfAddr(page) != page->region);

  
#ifdef OM_HAVE_TRACK
  if (! normal_page)
  {
    omCheckReturnCorrupted(! omIsSetTrackOfUsedBlocks(page->used_blocks));
  }
  else
#endif
    omAssume(normal_page);
  
  bin = omGetTopBinOfPage(page);
  if (bin->max_blocks > 1)
  {
    omCheckReturnCorrupted(omGetUsedBlocksOfPage(page) > bin->max_blocks - 1);
    omCheckReturnCorrupted(omGetUsedBlocksOfPage(page) == bin->max_blocks - 1 && 
                           page->current != NULL);
    omCheckReturnCorrupted(omGetUsedBlocksOfPage(page) < 0);
  }
  else
  {
    omCheckReturnCorrupted(omGetUsedBlocksOfPage(page) != 0);
  }
  
  omCheckReturn(omCheckList(page->current, level, report, OM_FLR_VAL));

  if (level > 1)
  {
    void* current = page->current;

    omCheckReturnCorrupted(current != NULL &&
                           omListLength(current) != bin->max_blocks - omGetUsedBlocksOfPage(page) - 1);
    
    while (current != NULL)
    {
      omCheckReturnCorrupted(omGetPageOfAddr(current) != page);
      
      omCheckReturnCorrupted( ( ( (unsigned long) current) 
                                - ((unsigned long) page) 
                                - SIZEOF_OM_BIN_PAGE_HEADER) 
                              % (bin->sizeW * SIZEOF_LONG)
                              != 0);
      current = *((void**) current);
    }
  }
  return omError_NoError;
}

omError_t omReportAddrError(omError_t error, omError_t report_error, void* addr, void* bin_size, omTrackFlags_t flags, 
                            OM_FLR_DECL, const char* fmt, ...) 
{
  va_list ap;
  va_start(ap, fmt);
  omReportError(error, report_error, OM_FLR_VAL, fmt, ap);
  
  _omPrintAddrInfo(stderr, error, addr, bin_size, flags, " occured for");
  return om_ErrorStatus;
}

void _omPrintAddrInfo(FILE* fd, omError_t error, void* addr, void* bin_size, omTrackFlags_t flags, char* s)
{
  if (! omCheckPtr(addr, omError_MaxError, OM_FLR))
  {
    fprintf(fd, "%s addr:%p size:%d", s, addr, omSizeOfAddr(addr));
  
  if (error == omError_WrongSize && (flags & OM_FSIZE))
    fprintf(fd, " specified size:%d", (size_t) bin_size);
  
  if (error == omError_WrongBin && (flags & OM_FBIN))
    fprintf(fd, " specified bin is of size:%d", ((omBin) bin_size)->sizeW << LOG_SIZEOF_LONG);
  
  if (omIsTrackAddr(addr)) 
    omPrintTrackAddrInfo(fd, addr);
  else
    fprintf(fd, "\n");
  }
  else
  {
    fprintf(fd, "%s (invalid) addr: %p\n", s, addr);
  }
}

void omPrintAddrInfo(FILE* fd, void *addr, char* s)
{
  _omPrintAddrInfo(fd, omError_NoError, addr, NULL, 0, s);
}

/*******************************************************************
 *  
 * Misc for iterating, etc.
 *  
 *******************************************************************/

void omIterateTroughBinAddrs(omBin bin, void (*CallBackUsed)(void*), void (*CallBackFree)(void*))
{
  omBinPage page;
  void* addr;
  void* is_free;
  int i;
  
  do
  {
    page = bin->last_page;
    while (page != NULL)
    {
      addr = (void*) page + SIZEOF_OM_BIN_PAGE_HEADER;
      i = 0;
      do
      {
        is_free = omIsOnList(page->current, addr);
        if (is_free)
        {
          if (CallBackFree != NULL) CallBackFree(addr);
        }
        else 
        {
          if (CallBackUsed != NULL) CallBackUsed(addr);
        }
        addr = (void**) addr + bin->sizeW;
        i++;
      } while (i < bin->max_blocks);
      page = page->prev;
    }
    bin = bin->next;
  } while (bin != NULL);
  
}

void omIterateTroughAddrs(int normal, int track, void (*CallBackUsed)(void*), void (*CallBackFree)(void*))
{
  int i;
  omSpecBin s_bin;

  if (normal)
  {
    for (i=0; i<=OM_MAX_BIN_INDEX; i++)
    {
      omIterateTroughBinAddrs(&om_StaticBin[i], CallBackUsed, CallBackFree);
    }
    s_bin = om_SpecBin;
    while (s_bin != NULL)
    {
      omIterateTroughBinAddrs(s_bin->bin, CallBackUsed, CallBackFree);
      s_bin = s_bin->next;
    }
  }
  
#ifdef OM_HAVE_TRACK
  if (track)
  {
    for (i=0; i<=OM_MAX_BIN_INDEX; i++)
    {
      omIterateTroughBinAddrs(&om_StaticTrackBin[i], CallBackUsed, CallBackFree);
    }
    s_bin = om_SpecTrackBin;
    while (s_bin != NULL)
    {
      omIterateTroughBinAddrs(s_bin->bin, CallBackUsed, CallBackFree);
      s_bin = s_bin->next;
    }
  }
#endif
}

static FILE* om_print_used_addr_fd;
static size_t om_total_used_size;
static unsigned int om_total_used_blocks;

static void _omPrintUsedAddr(void* addr)
{
  if (!omIsTrackAddr(addr) || !omIsStaticTrackAddr(addr))
  {
    om_total_used_blocks++;
    om_total_used_size += omSizeOfAddr(addr);
    _omPrintAddrInfo(om_print_used_addr_fd, omError_NoError, addr, NULL, 0, "");
    fprintf(om_print_used_addr_fd, "\n");
  }
}

void omPrintUsedAddrs(FILE* fd)
{
  om_total_used_size = 0;
  om_total_used_blocks = 0;
  om_print_used_addr_fd = (fd == NULL ? stdout : fd);
  omIterateTroughAddrs(1, 1, _omPrintUsedAddr, NULL);
  fprintf(fd, "UsedAddrs Summary: UsedBlocks:%d  TotalSize:%d\n", 
          om_total_used_blocks, om_total_used_size);
}

void omPrintUsedTrackAddrs(FILE* fd)
{
  om_total_used_size = 0;
  om_total_used_blocks = 0;
  om_print_used_addr_fd = (fd == NULL ? stdout : fd);
  omIterateTroughAddrs(0, 1 ,  _omPrintUsedAddr, NULL);
  fprintf(fd, "UsedTrackAddrs Summary: UsedBlocks:%d  TotalSize:%d\n", 
          om_total_used_blocks, om_total_used_size);
}

#endif /* ! OM_NDEBUG */
