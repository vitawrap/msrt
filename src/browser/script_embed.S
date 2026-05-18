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
embed "js_scripts/compiler.js", "__script_compiler_js"
embed "js_scripts/parser.js", "__script_parser_js"
embed "js_scripts/processor.js", "__script_processor_js"
embed "js_scripts/program.js", "__script_program_js"
embed "js_scripts/routine.js", "__script_routine_js"
embed "js_scripts/runner.js", "__script_runner_js"
embed "js_scripts/token.js", "__script_token_js"
embed "js_scripts/tokenizer.js", "__script_tokenizer_js"
embed "js_scripts/transpiler.js", "__script_transpiler_js"

embed "js_scripts/patch.js", "__script_patch_js"
