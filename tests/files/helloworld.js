/**
 * "type" : { "toString"  : "function void ()" },
 * "returnType" : { "toString"  : "void" }
 */
function helloWorld()
{
  alert("Hello World!");
}

/**
 * "EXPECT_FAIL" : { "type" : "int type is not properly deduced" },
 * "type" : { "toString" : "int" }
 */
var a = 5;

/**
 * "EXPECT_FAIL" : { "returnType" : "return type is not properly deduced" },
 * "returnType" : { "toString" : "String" }
 */
function testVariables()
{
 /**
  * "EXPECT_FAIL" : { "type" : "string type is not properly deduced" },
  * "type" : { "toString" : "String" }
  */
  var b = "some text";
  return b;
}

/**
 * "EXPECT_FAIL" : { "type" : "neither arg type nor ret type is properly deduced" },
 * "type": { "toString" : "function mixed (mixed)" }
 */
function testReturnMixedArg(arg)
{
    return arg;
}
