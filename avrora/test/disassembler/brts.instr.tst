; @Harness: disassembler
; @Result: PASS
  section .text  size=0x00000100 vma=0x00000000 lma=0x00000000 offset=0x00000034 ;2**0 
  section .data  size=0x00000000 vma=0x00000000 lma=0x00000000 offset=0x00000134 ;2**0 

start .text:

label 0x00000000  ".text":
      0x0: 0xfe 0xf1  brts  .+126  ;  0x80
      0x2: 0xf6 0xf1  brts  .+124  ;  0x80
      0x4: 0xee 0xf1  brts  .+122  ;  0x80
      0x6: 0xe6 0xf1  brts  .+120  ;  0x80
      0x8: 0xde 0xf1  brts  .+118  ;  0x80
      0xa: 0xd6 0xf1  brts  .+116  ;  0x80
      0xc: 0xce 0xf1  brts  .+114  ;  0x80
      0xe: 0xc6 0xf1  brts  .+112  ;  0x80
     0x10: 0xbe 0xf1  brts  .+110  ;  0x80
     0x12: 0xb6 0xf1  brts  .+108  ;  0x80
     0x14: 0xae 0xf1  brts  .+106  ;  0x80
     0x16: 0xa6 0xf1  brts  .+104  ;  0x80
     0x18: 0x9e 0xf1  brts  .+102  ;  0x80
     0x1a: 0x96 0xf1  brts  .+100  ;  0x80
     0x1c: 0x8e 0xf1  brts  .+98  ;  0x80
     0x1e: 0x86 0xf1  brts  .+96  ;  0x80
     0x20: 0x7e 0xf1  brts  .+94  ;  0x80
     0x22: 0x76 0xf1  brts  .+92  ;  0x80
     0x24: 0x6e 0xf1  brts  .+90  ;  0x80
     0x26: 0x66 0xf1  brts  .+88  ;  0x80
     0x28: 0x5e 0xf1  brts  .+86  ;  0x80
     0x2a: 0x56 0xf1  brts  .+84  ;  0x80
     0x2c: 0x4e 0xf1  brts  .+82  ;  0x80
     0x2e: 0x46 0xf1  brts  .+80  ;  0x80
     0x30: 0x3e 0xf1  brts  .+78  ;  0x80
     0x32: 0x36 0xf1  brts  .+76  ;  0x80
     0x34: 0x2e 0xf1  brts  .+74  ;  0x80
     0x36: 0x26 0xf1  brts  .+72  ;  0x80
     0x38: 0x1e 0xf1  brts  .+70  ;  0x80
     0x3a: 0x16 0xf1  brts  .+68  ;  0x80
     0x3c: 0x0e 0xf1  brts  .+66  ;  0x80
     0x3e: 0x06 0xf1  brts  .+64  ;  0x80
     0x40: 0xfe 0xf0  brts  .+62  ;  0x80
     0x42: 0xf6 0xf0  brts  .+60  ;  0x80
     0x44: 0xee 0xf0  brts  .+58  ;  0x80
     0x46: 0xe6 0xf0  brts  .+56  ;  0x80
     0x48: 0xde 0xf0  brts  .+54  ;  0x80
     0x4a: 0xd6 0xf0  brts  .+52  ;  0x80
     0x4c: 0xce 0xf0  brts  .+50  ;  0x80
     0x4e: 0xc6 0xf0  brts  .+48  ;  0x80
     0x50: 0xbe 0xf0  brts  .+46  ;  0x80
     0x52: 0xb6 0xf0  brts  .+44  ;  0x80
     0x54: 0xae 0xf0  brts  .+42  ;  0x80
     0x56: 0xa6 0xf0  brts  .+40  ;  0x80
     0x58: 0x9e 0xf0  brts  .+38  ;  0x80
     0x5a: 0x96 0xf0  brts  .+36  ;  0x80
     0x5c: 0x8e 0xf0  brts  .+34  ;  0x80
     0x5e: 0x86 0xf0  brts  .+32  ;  0x80
     0x60: 0x7e 0xf0  brts  .+30  ;  0x80
     0x62: 0x76 0xf0  brts  .+28  ;  0x80
     0x64: 0x6e 0xf0  brts  .+26  ;  0x80
     0x66: 0x66 0xf0  brts  .+24  ;  0x80
     0x68: 0x5e 0xf0  brts  .+22  ;  0x80
     0x6a: 0x56 0xf0  brts  .+20  ;  0x80
     0x6c: 0x4e 0xf0  brts  .+18  ;  0x80
     0x6e: 0x46 0xf0  brts  .+16  ;  0x80
     0x70: 0x3e 0xf0  brts  .+14  ;  0x80
     0x72: 0x36 0xf0  brts  .+12  ;  0x80
     0x74: 0x2e 0xf0  brts  .+10  ;  0x80
     0x76: 0x26 0xf0  brts  .+8  ;  0x80
     0x78: 0x1e 0xf0  brts  .+6  ;  0x80
     0x7a: 0x16 0xf0  brts  .+4  ;  0x80
     0x7c: 0x0e 0xf0  brts  .+2  ;  0x80
     0x7e: 0x06 0xf0  brts  .+0  ;  0x80
     0x80: 0xfe 0xf3  brts  .-2  ;  0x80
     0x82: 0xf6 0xf3  brts  .-4  ;  0x80
     0x84: 0xee 0xf3  brts  .-6  ;  0x80
     0x86: 0xe6 0xf3  brts  .-8  ;  0x80
     0x88: 0xde 0xf3  brts  .-10  ;  0x80
     0x8a: 0xd6 0xf3  brts  .-12  ;  0x80
     0x8c: 0xce 0xf3  brts  .-14  ;  0x80
     0x8e: 0xc6 0xf3  brts  .-16  ;  0x80
     0x90: 0xbe 0xf3  brts  .-18  ;  0x80
     0x92: 0xb6 0xf3  brts  .-20  ;  0x80
     0x94: 0xae 0xf3  brts  .-22  ;  0x80
     0x96: 0xa6 0xf3  brts  .-24  ;  0x80
     0x98: 0x9e 0xf3  brts  .-26  ;  0x80
     0x9a: 0x96 0xf3  brts  .-28  ;  0x80
     0x9c: 0x8e 0xf3  brts  .-30  ;  0x80
     0x9e: 0x86 0xf3  brts  .-32  ;  0x80
     0xa0: 0x7e 0xf3  brts  .-34  ;  0x80
     0xa2: 0x76 0xf3  brts  .-36  ;  0x80
     0xa4: 0x6e 0xf3  brts  .-38  ;  0x80
     0xa6: 0x66 0xf3  brts  .-40  ;  0x80
     0xa8: 0x5e 0xf3  brts  .-42  ;  0x80
     0xaa: 0x56 0xf3  brts  .-44  ;  0x80
     0xac: 0x4e 0xf3  brts  .-46  ;  0x80
     0xae: 0x46 0xf3  brts  .-48  ;  0x80
     0xb0: 0x3e 0xf3  brts  .-50  ;  0x80
     0xb2: 0x36 0xf3  brts  .-52  ;  0x80
     0xb4: 0x2e 0xf3  brts  .-54  ;  0x80
     0xb6: 0x26 0xf3  brts  .-56  ;  0x80
     0xb8: 0x1e 0xf3  brts  .-58  ;  0x80
     0xba: 0x16 0xf3  brts  .-60  ;  0x80
     0xbc: 0x0e 0xf3  brts  .-62  ;  0x80
     0xbe: 0x06 0xf3  brts  .-64  ;  0x80
     0xc0: 0xfe 0xf2  brts  .-66  ;  0x80
     0xc2: 0xf6 0xf2  brts  .-68  ;  0x80
     0xc4: 0xee 0xf2  brts  .-70  ;  0x80
     0xc6: 0xe6 0xf2  brts  .-72  ;  0x80
     0xc8: 0xde 0xf2  brts  .-74  ;  0x80
     0xca: 0xd6 0xf2  brts  .-76  ;  0x80
     0xcc: 0xce 0xf2  brts  .-78  ;  0x80
     0xce: 0xc6 0xf2  brts  .-80  ;  0x80
     0xd0: 0xbe 0xf2  brts  .-82  ;  0x80
     0xd2: 0xb6 0xf2  brts  .-84  ;  0x80
     0xd4: 0xae 0xf2  brts  .-86  ;  0x80
     0xd6: 0xa6 0xf2  brts  .-88  ;  0x80
     0xd8: 0x9e 0xf2  brts  .-90  ;  0x80
     0xda: 0x96 0xf2  brts  .-92  ;  0x80
     0xdc: 0x8e 0xf2  brts  .-94  ;  0x80
     0xde: 0x86 0xf2  brts  .-96  ;  0x80
     0xe0: 0x7e 0xf2  brts  .-98  ;  0x80
     0xe2: 0x76 0xf2  brts  .-100  ;  0x80
     0xe4: 0x6e 0xf2  brts  .-102  ;  0x80
     0xe6: 0x66 0xf2  brts  .-104  ;  0x80
     0xe8: 0x5e 0xf2  brts  .-106  ;  0x80
     0xea: 0x56 0xf2  brts  .-108  ;  0x80
     0xec: 0x4e 0xf2  brts  .-110  ;  0x80
     0xee: 0x46 0xf2  brts  .-112  ;  0x80
     0xf0: 0x3e 0xf2  brts  .-114  ;  0x80
     0xf2: 0x36 0xf2  brts  .-116  ;  0x80
     0xf4: 0x2e 0xf2  brts  .-118  ;  0x80
     0xf6: 0x26 0xf2  brts  .-120  ;  0x80
     0xf8: 0x1e 0xf2  brts  .-122  ;  0x80
     0xfa: 0x16 0xf2  brts  .-124  ;  0x80
     0xfc: 0x0e 0xf2  brts  .-126  ;  0x80
     0xfe: 0x06 0xf2  brts  .-128  ;  0x80

start .data:

