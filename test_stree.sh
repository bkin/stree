#!/bin/sh
oneTimeSetUp() {
  cat > input <<EOF
foo
bar
baz
EOF
}

oneTimeTearDown() {
  rm input
}

testBasic() {
  assertEquals "$(./stree input)" "
ba
bar
baz
foo"
}

testPrependFrequency() {
  assertEquals "$(./stree -f input)" \
"3        
2        ba
1        bar
1        baz
1        foo"
}

testAppendFrequency() {
  assertEquals "$(./stree -F input)" \
"3
ba 2
bar 1
baz 1
foo 1"
}

testPrependAndSort() {
  cat > input2 <<EOF
foo
bar
baz
folder
form
EOF
  # Writing frequency prints the most frequent on top
  assertEquals "$(./stree -f input2)" \
"5        
3        fo
1        folder
1        foo
1        form
2        ba
1        bar
1        baz"
  # Alphanumeric sort requires an extra parameter
  assertEquals "$(./stree -f -a input2)" \
"5        
2        ba
1        bar
1        baz
3        fo
1        folder
1        foo
1        form"
  rm input2
}

testSpaces() {
  assertEquals "$(./stree -s input)" \
"
ba
  r
  z
foo"
}

testParentheses() {
  assertEquals "$(./stree -p -s input)" "((ba(r)(z))(foo))"
}

. shunit2
