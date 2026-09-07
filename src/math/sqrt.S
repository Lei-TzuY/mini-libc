.text

.globl __mini_sqrt_hw
.type __mini_sqrt_hw, @function
__mini_sqrt_hw:
    sqrtsd %xmm0, %xmm0
    ret
.size __mini_sqrt_hw, .-__mini_sqrt_hw

.globl __mini_sqrtf_hw
.type __mini_sqrtf_hw, @function
__mini_sqrtf_hw:
    sqrtss %xmm0, %xmm0
    ret
.size __mini_sqrtf_hw, .-__mini_sqrtf_hw

.section .note.GNU-stack,"",@progbits
