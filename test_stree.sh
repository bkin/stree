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
  assertEquals "
ba
bar
baz
foo" "$(./stree input)"

  assertEquals "(foo)"         "$(echo foo | ./stree -p)"
  assertEquals "foo"           "$(echo foo | ./stree -b)"
  assertEquals "digraph {foo}" "$(echo foo | ./stree -g)"

  cat > foobar <<EOF
foo
bar
EOF
  assertEquals "((bar)(foo))"         "$(./stree -p foobar)"
  assertEquals "{bar,foo}"            "$(./stree -b foobar)"
  assertEquals "digraph {bar;foo}"    "$(./stree -g foobar)"
  rm foobar

  cat > barbaz <<EOF
bar
baz
EOF
  assertEquals "(ba(r)(z))"            "$(./stree -s -p barbaz)"
  assertEquals "ba{r,z}"               "$(./stree -s -b barbaz)"
  assertEquals "digraph {ba -> {r;z}}" "$(./stree -s -g barbaz)"
  rm barbaz
}

testPrependFrequency() {
  assertEquals \
"3       
2        ba
1        bar
1        baz
1        foo" "$(./stree -f input)"
}

testAppendFrequency() {
  assertEquals \
"3
ba 2
bar 1
baz 1
foo 1" "$(./stree -F input)"
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
  assertEquals \
"5       
3        fo
1        folder
1        foo
1        form
2        ba
1        bar
1        baz" "$(./stree -f input2)"
  # Alphanumeric sort requires an extra parameter
  assertEquals \
"5       
2        ba
1        bar
1        baz
3        fo
1        folder
1        foo
1        form" "$(./stree -f -a input2)"
  rm input2
}

testSpaces() {
  assertEquals \
"
ba
  r
  z
foo" "$(./stree -s input)"
}

testParentheses() {
  assertEquals "((ba(r)(z))(foo))" "$(./stree -p -s input)"
}

testBash() {
  assertEquals "{ba{r,z},foo}" "$(./stree -b -s input)"
}

testBash() {
  assertEquals "digraph {ba -> {r;z};foo}" "$(./stree -g -s input)"
}

. shunit2
