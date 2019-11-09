#include <iostream>
#include <map>
#include <vector>
#include <algorithm>
#include <fstream>
#include <math.h>
#include <string>
#include "pin.H"
// some code beloe borrows from tutorial presented by Artur Klauser @ Intel
#ifndef PIN_CACHE_H
#define PIN_CACHE_H


#define KILO 1024
#define M 1000000
#include <sstream>

/*!
 *  @brief Checks if n is a power of 2.
 *  @returns true if n is power of 2
 */
static inline bool IsPower2(UINT64 n)
{
    return ((n & (n - 1)) == 0);
}

/*!
 *  @brief Computes floor(log2(n))
 *  Works by finding position of MSB set.
 *  @returns -1 if n == 0.
 */
static inline UINT64 FloorLog2(UINT64 n)
{
    UINT64 p = 0;

    if (n == 0) return -1;
    if (n & 0xffffffff00000000) { p += 32; n >>= 32; }
    if (n & 0x00000000ffff0000) { p += 16; n >>= 16; }
    if (n & 0x000000000000ff00)	{ p +=  8; n >>=  8; }
    if (n & 0x00000000000000f0) { p +=  4; n >>=  4; }
    if (n & 0x000000000000000c) { p +=  2; n >>=  2; }
    if (n & 0x0000000000000002) { p +=  1; }

    return p;
}

/*!
 *  @brief Computes floor(log2(n))
 *  Works by finding position of MSB set.
 *  @returns -1 if n == 0.
 */
static inline INT32 CeilLog2(UINT64 n)
{
    return FloorLog2(n - 1) + 1;
}

/*!
 *  @brief Cache tag - self clearing on creation
 */
/*
class CACHE_TAG
{
  public:
    ADDRINT *_tag;
    CACHE_TAG(ADDRINT tag = 0) { *_tag = tag; }
    bool operator==(const CACHE_TAG &right) const { return ( *_tag ) == ( *right._tag ); }
    operator ADDRINT() const { return *_tag; }
};
*/

/*!
 *  @brief Cache set direct mapped
 */
class DIRECT_MAPPED_SET
{
  public:
    ADDRINT *tag;
    DIRECT_MAPPED_SET(UINT64 associativity = 1)
    { 
        ASSERTX(associativity == 1); 
        tag = new ADDRINT;
        *tag = 0;
    }

    VOID SetAssociativity(UINT64 associativity) { ASSERTX(associativity == 1); }
    UINT64 GetAssociativity(UINT64 associativity) { return 1; }

    UINT64 Find(ADDRINT in_tag)
    { 
        // cout << "Enter DM find\n";
        if( *tag == in_tag)
        {
            return 1;
        } 
        return 0;
    }
    VOID Replace(ADDRINT in_tag) 
    { 
        // cout << "orig " << (*tag) << " in_tag " << in_tag << "\n";
        *tag = in_tag; 
        // cout << "after replace " << (*tag) << "\n";
    }
};

typedef struct CACHE_NODE
{
    ADDRINT tag;
    struct CACHE_NODE *prev;
    struct CACHE_NODE *next;
}CACHE_NODE;

class LRU_SET
{
  private:
    UINT64 cur_size;
    UINT64 associativity;
    CACHE_NODE *node_head;
    CACHE_NODE *node_tail;
  public:
    LRU_SET( UINT64 associativity = 2 ) 
    { 
        associativity = 2;
        node_head = NULL;
        cur_size = 0;
        node_tail = NULL;
    }

    VOID SetAssociativity(UINT64 associativity) { ASSERTX(associativity == 1); }
    UINT64 GetAssociativity(UINT64 associativity) { return 1; }

    UINT64 Find( ADDRINT in_tag )
    { 
        
        if( node_head == NULL ) { return 0; }
        CACHE_NODE *cur = node_head;
        // cout << cur_size <<">>>>>>>>>>>>>>>>>>>>>>>cur_size\n";
        while( cur != NULL )
        {
            // cout << cur -> tag << "  " << in_tag << "\n";
            if( cur -> tag == in_tag )
            {
                return 1;
            }
            cur = cur -> next;
        }
        return( 0 ); 
    }

    VOID Replace( ADDRINT in_tag ) 
    { 
        CACHE_NODE *new_node = new CACHE_NODE();
        new_node -> prev = NULL;
        new_node -> next = NULL;
        new_node -> tag = in_tag;
        
        if( node_head == NULL )
        {
            node_head = new_node;
            node_tail = new_node;
            cur_size ++;
            
        }
        else
        {
            // cout << "haha\n";
            if( cur_size < 2 )
            {
                new_node -> next = node_head;
                node_head -> prev = new_node;
                node_head = new_node;
                cur_size ++;
            }
            else
            {   // LRU replacement
                new_node -> next = node_head;
                node_head -> prev = new_node;
                node_head = new_node;

                CACHE_NODE *tmp = node_tail;
                node_tail = node_tail -> prev;
                node_tail -> next = NULL;

                delete tmp;
            }   
        }
        // cout << node_head -> tag<<"\n";
    }
};

typedef enum 
{
    ACCESS_TYPE_LOAD,
    ACCESS_TYPE_STORE,
    ACCESS_TYPE_NUM
} ACCESS_TYPE;

typedef enum
{
    CACHE_TYPE_ICACHE,
    CACHE_TYPE_DCACHE,
    CACHE_TYPE_NUM
} CACHE_TYPE;


typedef enum 
{
    STORE_ALLOCATE,
    STORE_NO_ALLOCATE
} STORE_ALLOCATION;

typedef struct STAT_COUNTER
{
    long num_refer;
    long num_miss;
    ACCESS_TYPE type;
    ADDRINT pc;

}STAT_COUNTER;

std::map<ADDRINT, STAT_COUNTER *> *inst_stat_dict;
std::map<ADDRINT, STAT_COUNTER *> *data_stat_dict;

LRU_SET *lru_set_inst;
LRU_SET *lru_set_data;
DIRECT_MAPPED_SET* dm_set_inst;
DIRECT_MAPPED_SET* dm_set_data;

long *total_refer;
long *total_miss;
UINT64 *global_miss_penalty;

class CACHE
{
    public:
        UINT64 cache_size;
        UINT64 line_size;
        UINT64 associativity;

        UINT64 miss_penalty;
        UINT64 num_instr_cycle;
        UINT64 data_cache_stall;
        UINT64 instr_cache_stall;

        
        

        long instruction_exec;

        UINT64 _lineShift;
        UINT64 _setIndexMask;

        UINT64 con_cache_size;
        UINT64 con_line_size;
        string con_asso;
        UINT64 con_pen;

    CACHE( UINT64 in_cache, UINT64 in_line, UINT64 in_miss, UINT64 in_asso )
    {
        // cout << "Enter CACHE Construct\n";
        if( in_cache == 8 )
        {
            cache_size = 10 + 3;
        }
        else if( in_cache == 32 )
        {
            cache_size = 10 + 5;
        }
        else
        {
            cache_size = 0;
        }
        if( in_line == 64 ){ line_size = 6; }
        else if( in_line == 128 ){ line_size = 7; }
        miss_penalty = in_miss;
        associativity = in_asso;
        _lineShift = line_size;
        _setIndexMask = cache_size - line_size;
        if( in_asso == 2 )
        {
            _setIndexMask -= 1;
        }

        if( associativity == 1 )
        {
            dm_set_inst = new DIRECT_MAPPED_SET[ 1 << _setIndexMask ];
            dm_set_data = new DIRECT_MAPPED_SET[ 1 << _setIndexMask ];
        }
        else
        {
            lru_set_inst = new LRU_SET[ 1 << _setIndexMask ];
            lru_set_data = new LRU_SET[ 1 << _setIndexMask ];
        }
        *global_miss_penalty = in_miss;
        con_cache_size = in_cache;
        con_line_size = in_line;
        if( in_asso == 1 )
        {
            con_asso = "Direct Mapped";
        }
        else
        {
            con_asso = "2-way";
        }
        con_pen = in_miss;
        
        // cout << "Exit CACHE Construct\n";
    }

    bool is_dm()
    {
        // cout << "Enter is_dm\n";
        return associativity == 1;
    }

    ADDRINT GetTag(const ADDRINT addr)
    {
        ADDRINT tag = addr >> ( _setIndexMask +  _lineShift );
        return tag;
    }
    UINT64 GetSet( const ADDRINT addr )
    {
        UINT64 setIndex = ( addr >> _lineShift ) & ( ~0L + ( 1L << _setIndexMask ) );
        return setIndex;
    }

    UINT64 AccessInst( ADDRINT addr )
    {
        // cout << "Enter AccessInst " << std::hex << addr << "\n";
        std::map<ADDRINT, STAT_COUNTER *>::iterator it;

        ADDRINT tag = GetTag( addr );
        UINT64 setIndex = GetSet( addr );
        UINT64 localHit = 1;

        // SplitAddress( addr, tag, setIndex );
        // cout << "tag " << std::hex << tag << " "<< "setIndex " << setIndex << " setIndexMask " << _setIndexMask << "\n";
        if( is_dm() )
        {
            // cout << "begin\n";
            DIRECT_MAPPED_SET &set = dm_set_inst[ setIndex ];
            // cout << "End\n";
            // cout << "setIndex " << setIndex << " tag " << tag << " actual tag " << dm_set_inst[ setIndex ].tag << "\n"; 
            localHit = set.Find( tag );
            // cout << "localhit " << localHit << "\n";
            if ( localHit == 0 )
            {
                // cout << "replace";
                set.Replace(tag);
            }
        }
        else
        {
            LRU_SET &set = lru_set_inst[ setIndex ];
            
            localHit = set.Find( tag );
            if ( localHit == 0 )
            {
                set.Replace(tag);
            }
        }
        // increment stat
        it = (* inst_stat_dict ).find( addr );
        if( it == (* inst_stat_dict).end() )
        {
            (*inst_stat_dict )[ addr ] = new STAT_COUNTER();
            ( *inst_stat_dict )[ addr ] -> type =  ACCESS_TYPE_LOAD;
            ( *inst_stat_dict )[ addr ] -> pc = addr;
            ( *inst_stat_dict )[ addr ] -> num_refer = 1;
            ( *inst_stat_dict )[ addr ] -> num_miss = 1;
            ( *total_refer  ) ++;
            ( *total_miss ) ++;
        }
        else
        {
            ( *inst_stat_dict )[ addr ] -> num_refer ++;
            // cout << "after" << ( *inst_stat_dict )[ addr ] -> num_refer << "\n";
            (*total_refer) ++;
            if( localHit == 0 )
            {
                // cout << "Inst miss\n" ;
                // cout << "before" << ( *inst_stat_dict )[ addr ] -> num_miss << "\n";
                ( *inst_stat_dict )[ addr ] -> num_miss ++;
                // cout << "after" << ( *inst_stat_dict )[ addr ] -> num_miss << "\n";
                (*total_miss) ++;
            }
        }
        
        
        
        // cout << "Exit AccessInst\n";
        return localHit;
    }

    bool AccessData( ADDRINT addr, ACCESS_TYPE access_type )
    {
        // cout << "Enter AccessData\n";
        // const ADDRINT high_addr = addr + size;
        std::map<ADDRINT, STAT_COUNTER *>::iterator it;
        ADDRINT tag = GetTag( addr );
        UINT64 setIndex = GetSet( addr );
        UINT64 localHit = 1;

        // SplitAddress( addr, tag, setIndex );
        if( is_dm() )
        {
            DIRECT_MAPPED_SET &set = dm_set_data[ setIndex ];
            localHit = set.Find( tag );
            if ( localHit == 0 )
            {
                set.Replace(tag);
            }
        }
        else
        {
            // cout << "request" << tag << " " << setIndex << "\n";
            LRU_SET &set = lru_set_data[ setIndex ];
            localHit = set.Find( tag );
            if ( localHit == 0 )
            {
                set.Replace(tag);
            }
        }
        // increment stat
        // cout << "after cache\n";
        it = (*data_stat_dict).find( addr );
        if( it == (*data_stat_dict).end() )
        {
            (*data_stat_dict)[ addr ] = new STAT_COUNTER();
            (*data_stat_dict)[ addr ] -> type = access_type;
            (*data_stat_dict)[ addr ] -> pc = addr;
            ( *data_stat_dict )[ addr ] -> num_refer = 1;
            ( *data_stat_dict )[ addr ] -> num_miss = 1;
            ( *total_refer  ) ++;
            ( *total_miss ) ++;
        }
        else
        {
            // cout << "data before" << ( *data_stat_dict )[ addr ] -> num_refer << "\n";
            (*data_stat_dict)[ addr ] -> num_refer ++;
            // cout << "data after" << ( *data_stat_dict )[ addr ] -> num_refer << "\n";
            (*total_refer) ++;
            if( localHit == 0 )
            {
                (*data_stat_dict)[ addr ] -> num_miss ++;
                (*total_miss) ++;
            }
        }
        // cout << "Exit AccessData\n";
        return localHit;
    }
};

#endif // PIN_CACHE_H
// reference http://www.ic.unicamp.br/~rodolfo/mo801/04-PinTutorial.pdf
// Holds instruction count for a single procedure

CACHE *cache = NULL;

VOID mem_load(ADDRINT instId, ADDRINT addr)
{
    cache->AccessData(addr, ACCESS_TYPE_LOAD);
}
/* ===================================================================== */

VOID mem_store(ADDRINT instId, ADDRINT addr)
{
    cache->AccessData(addr, ACCESS_TYPE_STORE);
}

VOID access_inst( ADDRINT addr )
{
    // cout << std::hex << addr << "\n";
    
    cache -> AccessInst( addr );
}

VOID print( void *addr )
{
    printf( "ptr: %p\n", addr );
}

// Pin calls this function every time a new instruction is encountered
VOID Instruction(INS ins, VOID *v)
{
    // instruction analysis
    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)access_inst, IARG_INST_PTR, IARG_END);
    // INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)print, IARG_INST_PTR, IARG_END);
    
    UINT32 memOperands = INS_MemoryOperandCount(ins);

    // Iterate over each memory operand of the instruction.
    for (UINT32 memOp = 0; memOp < memOperands; memOp++)
    {
        if (INS_MemoryOperandIsRead(ins, memOp))
        {
            INS_InsertPredicatedCall(
                ins, IPOINT_BEFORE, (AFUNPTR)mem_load,
                IARG_INST_PTR,
                IARG_MEMORYOP_EA, memOp,
                IARG_END);
        }
        // Note that in some architectures a single memory operand can be 
        // both read and written (for instance incl (%eax) on IA-32)
        // In that case we instrument it once for read and once for write.
        if (INS_MemoryOperandIsWritten(ins, memOp))
        {
            INS_InsertPredicatedCall(
                ins, IPOINT_BEFORE, (AFUNPTR)mem_store,
                IARG_INST_PTR,
                IARG_MEMORYOP_EA, memOp,
                IARG_END);
        }
    }
    
    /*
    // data 
    if (INS_IsMemoryRead(ins))
    {
        // map sparse INS addresses to dense IDs
        // const ADDRINT iaddr = INS_Address(ins);
        // const UINT64 instId = profile.Map(iaddr);

        // const UINT64 size = INS_MemoryReadSize(ins);
        // const BOOL   single = (size <= 4);
        INS_InsertPredicatedCall(
            ins, IPOINT_BEFORE, (AFUNPTR) mem_load,
            IARG_MEMORYREAD_EA,
            IARG_END);
    }
        
    if ( INS_IsMemoryWrite(ins) )
    {
        // map sparse INS addresses to dense IDs
        // const ADDRINT iaddr = INS_Address(ins);
        // const UINT64 instId = profile.Map(iaddr);
            
        // const UINT64 size = INS_MemoryWriteSize(ins);

        // const BOOL   single = (size <= 4);
                
        INS_InsertPredicatedCall(
            ins, IPOINT_BEFORE,  (AFUNPTR) mem_store,
            IARG_MEMORYWRITE_EA,
            IARG_END);
    }
    */
}

// compare in descending order
bool comp( STAT_COUNTER *a, STAT_COUNTER *b )
{
    // return ( ( (double)( a -> num_miss ) / (double)( a -> num_refer ) ) > ( (double)( b -> num_miss ) / (double) ( b -> num_refer) ) ); //( ( (double)( a -> num_miss ) / (double)( a -> num_refer ) ) < ( (double)( b -> num_miss ) / (double) ( b -> num_refer) ) ) || ;
    return ( a -> num_miss > b -> num_miss  );
}

VOID Fini(int code, VOID * v)
{
    std::map<ADDRINT, STAT_COUNTER *>::iterator it;
    std::vector<STAT_COUNTER *>::iterator v_it;
    std::vector< STAT_COUNTER* > inst_stat;
    std::vector< STAT_COUNTER * > data_stat;
    std::map<ACCESS_TYPE, string> access_name_map;
    string sp = "        ";
    long inst_refer = 0;
    long data_refer = 0;
    long data_miss = 0;
    long inst_miss = 0;
    long total_exec = 0;
    int i;

    access_name_map[ ACCESS_TYPE_LOAD ] = "Load";
    access_name_map[ ACCESS_TYPE_STORE ] = "Store";

    for( it = inst_stat_dict -> begin(); it != inst_stat_dict -> end(); ++it )
    {
        inst_stat.push_back( it -> second );
    }
    
    for( it = data_stat_dict -> begin(); it != data_stat_dict -> end(); ++it )
    {
        data_stat.push_back( it -> second );
    }
    cout << "haha " << inst_stat[ 0 ] -> num_refer << "\n";
    cout << "haha " << inst_stat[ 0 ] -> num_miss << "\n";
    cout << "haha " << inst_stat[ 0 ] -> type << "\n";
    cout << "haha " << inst_stat[ 0 ] -> pc << "\n";
    std::sort( inst_stat.begin(), inst_stat.end(), comp );
    std::sort( data_stat.begin(), data_stat.end(), comp );
    cout << "after sort\n";
    

    for ( v_it = inst_stat.begin() ; v_it != inst_stat.end(); ++v_it)
    {
        inst_refer += ( ( * v_it ) -> num_refer );
        inst_miss += ( ( * v_it ) -> num_miss );
    }

    for ( v_it = data_stat.begin() ; v_it != data_stat.end(); ++v_it)
    {
        data_refer += ( ( * v_it ) -> num_refer );
        data_miss += ( ( * v_it ) -> num_miss );
    }

    cout << "after add\n";
    total_exec = ( inst_refer + data_miss * 100 + inst_miss * 100 );
    cout << "Overall Performance Breakdown:\n";
    cout << "==============================\n";
    cout << "Instruction Execution: "<< ( inst_refer ) << " cycles " << 100 * ( ( (double)inst_refer ) / ( (double)total_exec ) ) <<"%\n";
    cout << "Data Cache Stalls    : "<< ( data_miss * 100 ) << " cycles " << 100 * ( ( (double)( data_miss * 100 ) ) / ( (double)total_exec ) ) <<"%\n";
    cout << "Instruction Stalls   : "<< ( inst_miss * 100 ) << " cycles " << 100 * ( ( (double)( inst_miss * 100 ) ) / ( (double)total_exec ) ) <<"%\n";
    cout << "----------------------------------------------\n";
    cout << "Total execution time : " << total_exec << "\n";
    cout << "----------------------------------------------\n";
    cout << "total_refer " << (*total_refer) << " total_miss " << (*total_miss) << "\n";
    cout << "----------------------------------------------\n";
    cout << "\n\n";
    cout << "Instruction Config\n";
    cout << "-----------\n";
    cout << "Cache Size: " << cache->con_cache_size << "KB\n";
    cout << "Line Size: " << cache->con_line_size << "B\n";
    cout << "Associativity: " << cache->con_asso << "\n";
    cout << "Miss Penalty: " << cache->con_pen << " cycles\n";
    cout << "----------------------------------------------\n";
    cout << "References         : " << inst_refer << "\n";
    cout << "Misses             : " << inst_miss << "\n";
    cout << "Miss Rate          : " << ( (double)inst_miss ) / ( (double)inst_refer ) * 100.0 << "%\n";
    cout << "Instruction Stall  : " << inst_miss * 100 << "\n";


    cout << "\n\n";
    cout << "Ordered by absolute miss cycles\n";

    v_it = inst_stat.begin();
    cout << "Index    PC     References    Misses    Miss Rate     Miss Cycles    Contribution\n";
    for( i = 1; i < 21; i ++ )
    {
        // cout << (*inst_stat_dict)[ (*v_it) -> pc ] -> num_refer << "\n";
        cout << std::dec << i << sp << std::hex << ( (*v_it) -> pc ) << sp << std::dec << ( (*v_it) -> num_refer ) << sp << ( (*v_it) -> num_miss )<< sp << ( long double ) ( (*v_it) -> num_miss ) / ( long double ) ( (*v_it) -> num_refer ) << sp << ( (*v_it) -> num_miss ) * 100 << sp << ( double ) ( (*v_it) -> num_miss ) / ( double ) ( inst_miss ) << "\n";
        ++v_it;
        if( v_it == inst_stat.end() )
        {
            break;
        }
    }
    cout << "\n\n";
    cout << "Data Config\n";
    cout << "-----------\n";
    cout << "Cache Size: " << cache->con_cache_size << "KB\n";
    cout << "Line Size: " << cache->con_line_size << "B\n";
    cout << "Associativity: " << cache->con_asso << "\n";
    cout << "Miss Penalty: " << cache->con_pen << " cycles\n";
    cout << "----------------------------------------------\n";
    cout << "References         : " << data_refer << "\n";
    cout << "Misses             : " << data_miss << "\n";
    cout << "Miss Rate          : " << ( (double)data_miss ) / ( (double)data_refer ) * 100.0 << "%\n";
    cout << "Data Stall         : " << data_miss * 100 << "\n";
    cout << "\n\n";
    cout << "Ordered by absolute miss cycles\n";

    v_it = data_stat.begin();
    cout << "Index    PC    Type     References    Misses    Miss Rate     Miss Cycles    Contribution\n";
    for( i = 1; i < 21; i ++ )
    {
        // cout << (*data_stat_dict)[ (*v_it) -> pc ] -> num_refer << "\n";
        cout << std::dec << i << sp << std::hex << ( (*v_it) -> pc ) << sp << access_name_map[ (*v_it) -> type ] << sp << std::dec << ( (*v_it) -> num_refer ) << sp << ( (*v_it) -> num_miss )<< sp << ( long double ) ( (*v_it) -> num_miss ) / ( long double ) ( (*v_it) -> num_refer ) << sp << ( (*v_it) -> num_miss ) * 100 << sp << ( double ) ( (*v_it) -> num_miss ) / ( double ) ( data_miss ) << "\n";
        ++v_it;
        if( v_it == data_stat.end() )
        {
            break;
        }
    }
}

INT32 Usage()
{
    PIN_ERROR( "This Pintool prints a trace of memory addresses\n" 
              + KNOB_BASE::StringKnobSummary() + "\n");
    return -1;
}

int main(int argc, char * argv[])
{
    if (PIN_Init(argc, argv)) return Usage();
    // PIN_InitSymbols();
    total_refer = new long;
    total_miss = new long;
    global_miss_penalty = new UINT64;
    cache = new CACHE( 8, 128, 100, 1 );
    inst_stat_dict = new std::map<ADDRINT, STAT_COUNTER *>;
    data_stat_dict = new std::map<ADDRINT, STAT_COUNTER *>;
    
    INS_AddInstrumentFunction(Instruction, 0);
    PIN_AddFiniFunction(Fini, 0);
    PIN_StartProgram();
    
    return 0;
}