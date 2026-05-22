# Test CAS loading in openMSX
# Save as test_cas.tcl and run: openmsx -script test_cas.tcl

after time 3 "cassetteplayer insert \"C:\\msxgl\\projects\\nau_dx\\tape\\test_border.cas\""
after time 4 "type \"BLOAD\\\"CAS:\\\"\",R\""
after time 5 "type \"\n\""
after time 30 "screenshot \"C:\\msxgl\\projects\\nau_dx\\tape\\test_result.png\""
after time 35 "quit"
