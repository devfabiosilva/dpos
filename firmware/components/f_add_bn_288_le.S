; #
; #	AUTHOR: Fábio Pereira da Silva
; #	YEAR: 2019
; #	LICENSE: MIT
; #	EMAIL: fabioegel@gmail.com or fabioegel@protonmail.com
; #
; #Fri Aug 09 2019 18:27:10 -03
; ## Implementation of big number 288 bit used in Nano Cryptocurrency C Library for IA64 architechture

.text
.global f_add_bn_288_le
.align 4
.type f_add_bn_288_le, @function

f_add_bn_288_le:

   entry sp, 16

   mov.n a10, a2
   mov.n a11, a3
   mov.n a12, a4
   mov.n a13, a5
   mov.n a9,  a6

   l32i.n a2, a10, 0
   l32i.n a3, a10, 4
   l32i.n a4, a10, 8
   l32i.n a5, a10, 12

   movi.n a14, 0

f_add_bn_288_le_RET:

   add.n a2, a2, a9
   mov.n a8, a9
   movi.n a9, 1
   bltu a2, a8, k1
   movi.n a9, 0

k1:
   l32i.n a8, a11, 0
   add.n a2, a2, a8
   bnez a9, p1
   movi.n a9, 1
   bltu a2, a8, p1
   movi.n a9, 0

p1:
   add.n a3, a3, a9
   mov.n a8, a9
   movi.n a9, 1
   bltu a3, a8, p2
   movi.n a9, 0

p2:
   l32i.n a8, a11, 4
   add.n a3, a3, a8
   bnez a9, p3
   movi.n a9, 1
   bltu a3, a8, p3
   movi.n a9, 0

p3:
   add.n a4, a4, a9
   mov.n a8, a9
   movi.n a9, 1
   bltu a4, a8, p4
   movi.n a9, 0

p4:
   l32i a8, a11, 8
   add.n a4, a4, a8
   bnez a9, p5
   movi.n a9, 1
   bltu a4, a8, p5
   movi.n a9, 0

p5:
   add.n a5, a5, a9
   mov.n a8, a9
   movi.n a9, 1
   bltu a5, a8, p6
   movi.n a9, 0

p6:
   l32i a8, a11, 12
   add.n a5, a5, a8
   bnez a9, p7a
   movi.n a9, 1
   bltu a5, a8, p7a
   movi.n a9, 0

p7a:
   bnez a14, p7
   movi.n a14, 1
   addi a11, a11, 16

   s32i.n a2, a12, 0
   s32i.n a3, a12, 4
   s32i.n a4, a12, 8
   s32i.n a5, a12, 12

   l32i.n a2, a10, 16
   l32i.n a3, a10, 20
   l32i.n a4, a10, 24
   l32i.n a5, a10, 28
   l32i.n a6, a10, 32     
   j f_add_bn_288_le_RET

p7:
   add.n a6, a6, a9
   mov.n a8, a9
   movi.n a9, 1
   bltu a6, a8, p8
   movi.n a9, 0

p8:
   l32i a8, a11, 16
   add.n a6, a6, a8

   beqz a13, p10

   bnez a9, p9
   movi.n a9, 1
   bltu a6, a8, p9
   movi.n a9, 0

p9:
   s32i.n a9, a13, 0

p10:
   s32i.n a2, a12, 16
   s32i.n a3, a12, 20
   s32i.n a4, a12, 24
   s32i.n a5, a12, 28
   s32i.n a6, a12, 32
   retw.n

.size f_add_bn_288_le, .-f_add_bn_288_le

.text
.global f_sl_elv_add_le
.align 4
.type f_sl_elv_add_le, @function

f_sl_elv_add_le:

   entry sp, 16
   mov.n a11, a2
   mov.n a12, a3

   l32i a2, a11, 0
   l32i a3, a11, 4
   l32i a4, a11, 8
   l32i a5, a11, 12
   l32i a6, a11, 16
   l32i a7, a11, 20
   l32i a8, a11, 24
   l32i a9, a11, 28
   l32i a10, a11, 32

   slli a10, a10, 11
   mov.n a13, a9
   slli a9, a9, 11
   extui a14, a13, 21, 11
   or a10, a10, a14
   mov.n a13, a8
   slli a8, a8, 11
   extui a14, a13, 21, 11
   or a9, a9, a14
   mov.n a13, a7
   slli a7, a7, 11
   extui a14, a13, 21, 11
   or a8, a8, a14
   mov.n a13, a6
   slli a6, a6, 11
   extui a14, a13, 21, 11
   or a7, a7, a14
   mov.n a13, a5
   slli a5, a5, 11
   extui a14, a13, 21, 11
   or a6, a6, a14
   mov.n a13, a4
   slli a4, a4, 11
   extui a14, a13, 21, 11
   or a5, a5, a14
   mov.n a13, a3
   slli a3, a3, 11
   extui a14, a13, 21, 11
   or a4, a4, a14
   mov.n a13, a2
   slli a2, a2, 11
   extui a14, a13, 21, 11
   or a3, a3, a14
   or a2, a2, a12

   s32i a2, a11, 0
   s32i a3, a11, 4
   s32i a4, a11, 8
   s32i a5, a11, 12
   s32i a6, a11, 16
   s32i a7, a11, 20
   s32i a8, a11, 24
   s32i a9, a11, 28
   s32i a10, a11, 32

   retw.n

.size f_sl_elv_add_le, .-f_sl_elv_add_le


