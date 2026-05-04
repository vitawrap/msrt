/*
 * convenience assembly file to embed js scripts
 * in executable, bypassing compiler inspection
 */

.section .rodata

.macro embed file:req, global_name:req
    .global \global_name
    .type   \global_name, @object
    .balign 4
\global_name:
    .incbin "\file"
\global_name\()_end:

    .global \global_name\()_size
    .type   \global_name\()_size, @object
    .balign 4
\global_name\()_size:
    .int    \global_name\()_end - \global_name
.endm

/* all file embeds go here */
embed "js_scripts/play.js", "__script_play_js"
embed "js_scripts/patch.js", "__script_patch_js"
