/*
 * convenience assembly file to embed js scripts
 * in executable, bypassing compiler inspection
 */

#if defined(__unix__)
.section .rodata
#else
.section .rdata
#endif

.macro embed file:req, global_name:req
    .global \global_name
#if defined(__unix__)
    .type   \global_name, @object
#endif
    .balign 4
\global_name:
    .incbin "\file"
\global_name\()_end:

    .global \global_name\()_size
#if defined(__unix__)
    .type   \global_name\()_size, @object
#endif
    .balign 4
\global_name\()_size:
    .int    \global_name\()_end - \global_name
.endm

/* all file embeds go here */

/* SCRIPTS */
embed "assets/play.js", "__script_play_js"
embed "assets/compiler.js", "__script_compiler_js"
embed "assets/parser.js", "__script_parser_js"
embed "assets/processor.js", "__script_processor_js"
embed "assets/program.js", "__script_program_js"
embed "assets/routine.js", "__script_routine_js"
embed "assets/runner.js", "__script_runner_js"
embed "assets/token.js", "__script_token_js"
embed "assets/tokenizer.js", "__script_tokenizer_js"
embed "assets/transpiler.js", "__script_transpiler_js"

embed "assets/patch.js", "__script_patch_js"

/* FONTS */
embed "assets/bitcell.ttf", "__font_bitcell_ttf"
