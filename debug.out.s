	.text
	.globl main
	.attribute arch, "rv64i2p1_m2p0_a2p1_f2p2_d2p2_c2p0_zicsr2p0_zifencei2p0_zba1p0_zbb1p0"
main:
.main_0:
	sd			ra,-200(sp)
	fsd			fs11,-192(sp)
	fsd			fs10,-184(sp)
	fsd			fs9,-176(sp)
	fsd			fs8,-168(sp)
	fsd			fs7,-160(sp)
	fsd			fs6,-152(sp)
	fsd			fs5,-144(sp)
	fsd			fs4,-136(sp)
	fsd			fs3,-128(sp)
	fsd			fs2,-120(sp)
	fsd			fs1,-112(sp)
	fsd			fs0,-104(sp)
	sd			s11,-96(sp)
	sd			s10,-88(sp)
	sd			s9,-80(sp)
	sd			s8,-72(sp)
	sd			s7,-64(sp)
	sd			s6,-56(sp)
	sd			s5,-48(sp)
	sd			s4,-40(sp)
	sd			s3,-32(sp)
	sd			s2,-24(sp)
	sd			s1,-16(sp)
	sd			fp,-8(sp)
	add			fp,sp,x0
	li			t0,208
	sub			sp,sp,t0
	jal			x0,.main_1
.main_1:
	addiw		t0,x0,66
	addiw		t1,x0,0
	addw		t0,t0,t1
	sw			t0,0(sp)
	lw			t0,0(sp)
	add			a0,t0,x0
	call		putint
	lw			t0,0(sp)
	add			a0,t0,x0
	li			t0,208
	add			sp,sp,t0
	ld			fp,-8(sp)
	ld			s1,-16(sp)
	ld			s2,-24(sp)
	ld			s3,-32(sp)
	ld			s4,-40(sp)
	ld			s5,-48(sp)
	ld			s6,-56(sp)
	ld			s7,-64(sp)
	ld			s8,-72(sp)
	ld			s9,-80(sp)
	ld			s10,-88(sp)
	ld			s11,-96(sp)
	fld			fs0,-104(sp)
	fld			fs1,-112(sp)
	fld			fs2,-120(sp)
	fld			fs3,-128(sp)
	fld			fs4,-136(sp)
	fld			fs5,-144(sp)
	fld			fs6,-152(sp)
	fld			fs7,-160(sp)
	fld			fs8,-168(sp)
	fld			fs9,-176(sp)
	fld			fs10,-184(sp)
	fld			fs11,-192(sp)
	ld			ra,-200(sp)
	jalr		x0,ra,0
	.data
a:
	.word	1
b:
	.word	0
c:
	.word	1
d:
	.word	2
e:
	.word	4
