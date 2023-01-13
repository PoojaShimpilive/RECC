.section .rodata.str,"aMS",@progbits,1
.align 1
.align 1
c1gm_str:
        .asciz "hello"
.section .text
.align 8
.align 8
        .quad   0
        .long   21
        .long   ghczmprim_GHCziCString_unpackCStringzh_closure-(.Ls1gf_info)+0
.Ls1gf_info:
.Lc1gn:
        leaq -16(%rbp),%rax
        cmpq %r15,%rax
        jb .Lc1go
.Lc1gp:
        subq $8,%rsp
        movq %r13,%rax
        movq %rbx,%rsi
        movq %rax,%rdi
        xorl %eax,%eax
        call newCAF
        addq $8,%rsp
        testq %rax,%rax
        je .Lc1gl
.Lc1gk:
        movq $stg_bh_upd_frame_info,-16(%rbp)
        movq %rax,-8(%rbp)
        movl $c1gm_str,%r14d
        movl $ghczmprim_GHCziCString_unpackCStringzh_closure,%ebx
        addq $-16,%rbp
        jmp stg_ap_n_fast
.Lc1gl:
        jmp *(%rbx)
.Lc1go:
        jmp *-16(%r13)
        .size .Ls1gf_info, .-.Ls1gf_info
.section .data
.align 8
.align 1
.Ls1gf_closure:
        .quad   .Ls1gf_info
        .quad   0
        .quad   0
        .quad   0
.section .data
.align 8
.align 1
.Lu1gG_srt:
        .quad   stg_SRT_2_info
        .quad   base_SystemziIO_putStrLn_closure
        .quad   .Ls1gf_closure
        .quad   0
.section .text
.align 8
.align 8
        .quad   0
        .long   21
        .long   .Lu1gG_srt-(Main_main_info)+0
.globl Main_main_info
.type Main_main_info, @function
Main_main_info:
.Lc1gD:
        leaq -16(%rbp),%rax
        cmpq %r15,%rax
        jb .Lc1gE
.Lc1gF:
        subq $8,%rsp
        movq %r13,%rax
        movq %rbx,%rsi
        movq %rax,%rdi
        xorl %eax,%eax
        call newCAF
        addq $8,%rsp
        testq %rax,%rax
        je .Lc1gC
.Lc1gB:
        movq $stg_bh_upd_frame_info,-16(%rbp)
        movq %rax,-8(%rbp)
        movl $.Ls1gf_closure,%r14d
        movl $base_SystemziIO_putStrLn_closure,%ebx
        addq $-16,%rbp
        jmp stg_ap_p_fast
.Lc1gC:
        jmp *(%rbx)
.Lc1gE:
        jmp *-16(%r13)
        .size Main_main_info, .-Main_main_info
.section .data
.align 8
.align 1
.globl Main_main_closure
.type Main_main_closure, @object
Main_main_closure:
        .quad   Main_main_info
        .quad   0
        .quad   0
        .quad   0
.section .data
.align 8
.align 1
.Lu1gW_srt:
        .quad   stg_SRT_2_info
        .quad   base_GHCziTopHandler_runMainIO_closure
        .quad   Main_main_closure
        .quad   0
.section .text
.align 8
.align 8
        .quad   0
        .long   21
        .long   .Lu1gW_srt-(ZCMain_main_info)+0
.globl ZCMain_main_info
.type ZCMain_main_info, @function
ZCMain_main_info:
.Lc1gT:
        leaq -16(%rbp),%rax
        cmpq %r15,%rax
        jb .Lc1gU
.Lc1gV:
        subq $8,%rsp
        movq %r13,%rax
        movq %rbx,%rsi
        movq %rax,%rdi
        xorl %eax,%eax
        call newCAF
        addq $8,%rsp
        testq %rax,%rax
        je .Lc1gS
.Lc1gR:
        movq $stg_bh_upd_frame_info,-16(%rbp)
        movq %rax,-8(%rbp)
        movl $Main_main_closure,%r14d
        movl $base_GHCziTopHandler_runMainIO_closure,%ebx
        addq $-16,%rbp
        jmp stg_ap_p_fast
.Lc1gS:
        jmp *(%rbx)
.Lc1gU:
        jmp *-16(%r13)
        .size ZCMain_main_info, .-ZCMain_main_info
.section .data
.align 8
.align 1
.globl ZCMain_main_closure
.type ZCMain_main_closure, @object
ZCMain_main_closure:
        .quad   ZCMain_main_info
        .quad   0
        .quad   0
        .quad   0
.section .rodata.str,"aMS",@progbits,1
.align 1
.align 1
.Lr13l_bytes:
        .asciz "main"
.section .data
.align 8
.align 1
.Lr13D_closure:
        .quad   ghczmprim_GHCziTypes_TrNameS_con_info
        .quad   .Lr13l_bytes
.section .rodata.str,"aMS",@progbits,1
.align 1
.align 1
.Lr13E_bytes:
        .asciz "Main"
.section .data
.align 8
.align 1
.Lr13F_closure:
        .quad   ghczmprim_GHCziTypes_TrNameS_con_info
        .quad   .Lr13E_bytes
.section .data
.align 8
.align 1
.globl Main_zdtrModule_closure
.type Main_zdtrModule_closure, @object
Main_zdtrModule_closure:
        .quad   ghczmprim_GHCziTypes_Module_con_info
        .quad   .Lr13D_closure+1
        .quad   .Lr13F_closure+1
        .quad   3
.section .note.GNU-stack,"",@progbits
.ident "GHC 8.8.3"
