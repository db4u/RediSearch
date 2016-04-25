#include <stdio.h>
#include "varint.h"
#include "index.h"
#include "buffer.h"
#include <assert.h>
#include <math.h>
#include <time.h>
#include "tokenize.h"

#define TESTFUNC(f) \
                printf("Testing %s ...\t", __STRING(f)); \
                fflush(stdout); \
                if (f()) {printf ("FAIL\n"); exit(1); } else printf("PASS\n");
                
#define ASSERTM(expr, ...) if (!(expr)) { fprintf (stderr, "Assertion '%s' Failed: " __VA_ARGS__ "\n", __STRING(expr)); return -1; }
#define ASSERT(expr) if (!(expr)) { fprintf (stderr, "Assertion '%s' Failed\n", __STRING(expr)); return -1; }
#define ASSERT_EQUAL_INT(x,y,...) if (x!=y) { fprintf (stderr, "%d != %d: " __VA_ARGS__ "\n", x, y); return -1; }
                    
#define TEST_START() printf("Testing %s... ",  __FUNCTION__); fflush(stdout);
#define TEST_END() printf ("PASS!");


int testVarint() {
  VarintVectorWriter *vw = NewVarintVectorWriter(8);
  int expected[5] = {10, 1000,1020, 10000, 10020};
  for (int  i = 0; i < 5; i++) {
    VVW_Write(vw, expected[i]);    
  }
  
  
  // VVW_Write(vw, 100);
  printf("%ld %ld\n", BufferLen(vw->bw.buf), vw->bw.buf->cap);
  VVW_Truncate(vw);
  BufferSeek(vw->bw.buf, 0);
  VarintVectorIterator it = VarIntVector_iter(vw->bw.buf);
  int x = 0;
  
  
  while (VV_HasNext(&it)) {
    int n = VV_Next(&it);
    ASSERTM(n == expected[x++], "Wrong number decoded");
    printf("%d %d\n", x, n);
  }


  VVW_Free(vw);
  return 0;
}

int testDistance() {
    VarintVectorWriter *vw = NewVarintVectorWriter(8);
     VarintVectorWriter *vw2 = NewVarintVectorWriter(8);
    VVW_Write(vw, 1);
    VVW_Write(vw2, 4);
    
    //VVW_Write(vw, 9);
    VVW_Write(vw2, 7);
    
    VVW_Write(vw, 9);
    VVW_Write(vw, 13);
    
    VVW_Write(vw, 16);
    VVW_Write(vw, 22);
    VVW_Truncate(vw);
    VVW_Truncate(vw2);
    
    
    VarintVector *v[2] = {vw->bw.buf, vw2->bw.buf};
    int delta = VV_MinDistance(v, 2);
    printf("%d\n", delta);
    
    VVW_Free(vw);
    VVW_Free(vw2);
    
    
    return 0;
    
    
}



int testIndexReadWrite() {
  IndexWriter *w = NewIndexWriter(10000);

  for (int i = 0; i < 100; i++) {
    // if (i % 10000 == 1) {
    //     printf("iw cap: %ld, iw size: %d, numdocs: %d\n", w->cap, IW_Len(w),
    //     w->ndocs);
    // }
    IndexHit h;
    h.docId = i;
    h.flags = 0;
    h.freq = i % 10;

    VarintVectorWriter *vw = NewVarintVectorWriter(8);
    for (int n = 0; n < i % 4; n++) {
      VVW_Write(vw, n);
    }
    VVW_Truncate(vw);
    
    h.offsets = *vw->bw.buf;

    IW_Write(w, &h);
    VVW_Free(vw);
  }

  printf("iw cap: %ld, iw size: %ld, numdocs: %d\n", w->bw.buf->cap, IW_Len(w),
         w->ndocs);
  IW_Close(w);
  IW_MakeSkipIndex(w, 10);
  
//   for (int x = 0; x < w->skipIdx.len; x++) {
//     printf("Skip entry %d: %d, %d\n", x, w->skipIdx.entries[x].docId, w->skipIdx.entries[x].offset);
//   }
  printf("iw cap: %ld, iw size: %ld, numdocs: %d\n", w->bw.buf->cap, IW_Len(w),
         w->ndocs);

  
  IndexHit h;
  int n = 0;

  for (int xx = 0; xx < 1; xx++) {
    IndexReader *ir = NewIndexReader(w->bw.buf->data, w->bw.buf->cap, &w->skipIdx);
    IndexHit h;

    struct timespec start_time, end_time;
    for (int z= 0; z < 10; z++) {
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start_time);

    IR_SkipTo(ir, 900001, &h);
    
    
    
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end_time);
    long diffInNanos = end_time.tv_nsec - start_time.tv_nsec;
    

    printf("Time elapsed: %ldnano\n", diffInNanos);
    //IR_Free(ir);
    }
  }
  IW_Free(w);
  
  return 0;
}

IndexWriter *createIndex(int size, int idStep) {
   IndexWriter *w = NewIndexWriter(100);
   
  t_docId id = idStep;
  for (int i = 0; i < size; i++) {
    // if (i % 10000 == 1) {
    //     printf("iw cap: %ld, iw size: %d, numdocs: %d\n", w->cap, IW_Len(w),
    //     w->ndocs);
    // }
    IndexHit h;
    h.docId = id;
    h.flags = 0;
    h.freq = i % 10;
    
    VarintVectorWriter *vw = NewVarintVectorWriter(8);
    for (int n = idStep; n < idStep + i % 4; n++) {
      VVW_Write(vw, n);
    }
    VVW_Truncate(vw);
    h.offsets = *vw->bw.buf;

    IW_Write(w, &h);
    VVW_Free(vw);
    id += idStep;
  }


   printf("BEFORE: iw cap: %ld, iw size: %zd, numdocs: %d\n", w->bw.buf->cap, IW_Len(w),
         w->ndocs);
  
  
  w->bw.Truncate(w->bw.buf, 0);
  
  IW_MakeSkipIndex(w, 100);
  IW_Close(w);
  
  printf("iw cap: %ld, iw size: %zd, numdocs: %d\n", w->bw.buf->cap, IW_Len(w),
         w->ndocs);
  return w;
}


typedef struct {
    int maxFreq;
    int counter;
} IterationContext;

int onIntersect(void *ctx, IndexHit *hits, int argc) {
    
    //printf("%d\n", hits[0].docId);
    IterationContext *ic = ctx;
    ++ic->counter;
    VarintVector *viv[argc];
    double score = 0;
    for (int i =0; i < argc; i++) {
        viv[i] = &hits[i].offsets;
        score += log((double)hits[i].freq+2);
        
        
        // if (hits[i].freq > ic->maxFreq) 
        //     ic->maxFreq = hits[i].freq;
    }
    
    
    
    //int dist = VV_MinDistance(viv, argc);
    //score /= pow ((double)(dist+1), 2.0);
    //printf("%lf %d %lf\n", score, dist, score/pow ((double)(dist+1), 2.0) );
    return 0;
}

int printIntersect(void *ctx, IndexHit *hits, int argc) {
    
    printf("intersect: %d\n", hits[0].docId);
    return 0;
}

int testReadIterator() {
    IndexWriter *w = createIndex(10, 1);
    
    
    IndexReader *r1 = NewIndexReaderBuf(w->bw.buf, NULL);
    IndexHit h;
        
    IndexIterator *it = NewIndexIterator(r1);
    int i = 1;
    while(it->HasNext(it->ctx)) {
        if (it->Read(it->ctx, &h) == INDEXREAD_EOF) {
            return -1;
        }
        ASSERT(h.docId == i++);
        //printf("Iter got %d\n", h.docId);
    }
    return i == 11 ? 0 : -1;
}


int testUnion() {
    IndexWriter *w = createIndex(10, 2);
    IndexReader *r1 = NewIndexReader(w->bw.buf->data,  IW_Len(w), &w->skipIdx);
    IndexWriter *w2 = createIndex(10, 3);
    IndexReader *r2 = NewIndexReader(w2->bw.buf->data , IW_Len(w2), &w2->skipIdx);
    printf("Reading!\n");
    IndexIterator *irs[] = {NewIndexIterator(r1), NewIndexIterator(r2)};
    IndexIterator *ui =  NewUnionIterator(irs, 2);
    IndexHit h;
    int expected[] = {2, 3, 4, 6, 8, 9, 10, 12, 14, 15, 16, 18, 20, 21, 24, 27, 30};
    int i = 0;
    while (ui->Read(ui->ctx, &h) != INDEXREAD_EOF) {
        ASSERT(h.docId == expected[i++]);
         //printf("%d, ", h.docId);
    }
    // IndexWriter *w3 = createIndex(30, 5);
    // IndexReader *r3 = NewIndexReader(w3->bw.buf->data,  IW_Len(w3), &w3->skipIdx);
    
    
    // IndexIterator *irs2[] = {ui, NewIndexIterator(r3)};
    // // while (ui->Read(ui->ctx, &h) != INDEXREAD_EOF) {
    // //     printf("Read %d\n", h.docId);
    // // }
    //  IterationContext ctx = {0,0};
    // int count = IR_Intersect2(irs2, 2, printIntersect, &ctx);
    // printf("%d\n", count);
    ui->Free(ui);
    return 0;
}

int testIntersection() {
    
    IndexWriter *w = createIndex(100000, 4);
    IndexReader *r1 = NewIndexReader(w->bw.buf->data,  IW_Len(w), &w->skipIdx);
    IndexWriter *w2 = createIndex(100000, 2);
    IndexReader *r2 = NewIndexReader(w2->bw.buf->data,  IW_Len(w2), &w2->skipIdx);
    
    // IndexWriter *w3 = createIndex(10000, 3);
    // IndexReader *r3 = NewIndexReader(w3->bw.buf->data,  IW_Len(w3), &w3->skipIdx);
    
    IterationContext ctx = {0,0};
    
    
    IndexIterator *irs[] = {NewIndexIterator(r1), NewIndexIterator(r2)};//,NewIndexTerator(r2)};
    
    printf ("Intersecting...\n");
    
    int count = 0;
    IndexIterator *ii = NewIntersecIterator(irs, 2, 0);
    struct timespec start_time, end_time;
    clock_gettime(CLOCK_REALTIME, &start_time);
    IndexHit h;
    while (ii->Read(ii->ctx, &h) != INDEXREAD_EOF) {
        //printf("%d\n", h.docId);
        ++count;
    }    
    
    //int count = IR_Intersect(r1, r2, onIntersect, &ctx);
    clock_gettime(CLOCK_REALTIME, &end_time);
    long diffInNanos = end_time.tv_nsec - start_time.tv_nsec;

    printf("%d intersections in %ldns\n", count, diffInNanos); 
    printf("top freq: %d\n", ctx.maxFreq);
    ASSERT(count == 50000)
    return 0;
}

int testMemBuffer() {
    //TEST_START();
    
    BufferWriter w = NewBufferWriter(2);
    ASSERTM( w.buf->cap == 2, "Wrong capacity");
    ASSERT (w.buf->data != NULL);
    ASSERT( BufferLen(w.buf) == 0);
    ASSERT( w.buf->data == w.buf->pos );
    return 0;
    
    const char *x = "helo";
    size_t l = w.Write(w.buf, (void*)x, strlen(x)+1);
    
    ASSERT( l == strlen(x)+1);
    ASSERT( BufferLen(w.buf) == l);
    ASSERT (w.buf->cap == 8);
    
    l = WriteVarint(1337, &w);
    ASSERT (l == 2);
    ASSERT( BufferLen(w.buf) == 7);
    ASSERT( w.buf->cap == 8);
    
    w.Truncate(w.buf, 0);
    ASSERT( w.buf->cap == 7);
    
    Buffer *b = NewBuffer(w.buf->data, w.buf->cap, BUFFER_READ);
    ASSERT(b->cap = w.buf->cap);
    ASSERT(b->data = w.buf->data);
    ASSERT(b->pos == b->data);
    
    
    char *y = malloc(5);
    l = BufferRead(b, y, 5);
    ASSERT(l == l);
    
    ASSERT( strcmp(y, "helo") == 0 );
    ASSERT( BufferLen(b) == 5);
    
    free(y);
    
    int n = ReadVarint(b);
    ASSERT (n == 1337);
    
    w.Release(w.buf);
    //TEST_END();
    return 0;
}


typedef struct{
    int num;
    char **expected;
    
} tokenContext;

int tokenFunc(void *ctx, Token t) {
    tokenContext *tx = ctx;
    
    assert( strcmp(t.s, tx->expected[tx->num++]) == 0);
    assert(t.len == strlen(t.s));
    assert(t.fieldId == 1);
    assert(t.pos > 0);
    assert(t.score == 1);
    return 0;    
}

int testTokenize() {
    
    char *txt = strdup("Hello? world...   ? __WAZZ@UP? שלום");
    tokenContext ctx = {0};
    const char *expected[] = {"hello", "world", "wazz", "up", "שלום"};
    ctx.expected = (char **)expected;
    
    tokenize(txt, 1, 1, &ctx, tokenFunc);
    ASSERT(ctx.num == 5);
    
    free(txt);
    
    return 0;
    
}

int testForwardIndex() {
    
    ForwardIndex *idx = NewForwardIndex(1,  1.0);
    char *txt = strdup("Hello? world...  hello hello ? __WAZZ@UP? שלום");
    tokenize(txt, 1, 1, idx, forwardIndexTokenFunc);

    return 0;
}


int main(int argc, char **argv) {
  
    LOGGING_LEVEL = L_DEBUG;
    TESTFUNC(testVarint);
    TESTFUNC(testDistance);
    TESTFUNC(testIndexReadWrite);
     TESTFUNC(testIntersection);
    TESTFUNC(testReadIterator);
    TESTFUNC(testUnion);

    TESTFUNC(testMemBuffer);
    TESTFUNC(testTokenize);
    TESTFUNC(testForwardIndex);
  return 0;
}