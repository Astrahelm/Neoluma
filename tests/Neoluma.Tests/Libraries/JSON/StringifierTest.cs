using Neoluma.Libraries;
namespace Neoluma.Tests.Libraries;

public class StringifierTest {
    [Fact]
    public void stringifyPrimitiveValues() {
        Assert.Equal("null", JSON.stringify(new JSON.Value()));
        Assert.Equal("42", JSON.stringify(new JSON.Value(42)));
        Assert.Equal("-42", JSON.stringify(new JSON.Value(-42)));
        Assert.Equal("1.5", JSON.stringify(new JSON.Value(1.5)));
        Assert.Equal("true", JSON.stringify(new JSON.Value(true)));
        Assert.Equal("false", JSON.stringify(new JSON.Value(false)));
        Assert.Equal("\"test\"", JSON.stringify(new JSON.Value("test")));
    }

    [Fact]
    public void stringifySpecialNumbers() {
        Assert.Equal("NaN", JSON.stringify(new JSON.Value(double.NaN)));
        Assert.Equal("Infinity", JSON.stringify(new JSON.Value(double.PositiveInfinity)));
        Assert.Equal("-Infinity", JSON.stringify(new JSON.Value(double.NegativeInfinity)));
    }

    [Fact]
    public void stringifyStringEscapes() {
        Assert.Equal("\"\\\"\"", JSON.stringify(new JSON.Value("\"")));
        Assert.Equal("\"\\\\\"", JSON.stringify(new JSON.Value("\\")));
        Assert.Equal("\"\\b\\f\\n\\r\\t\"", JSON.stringify(new JSON.Value("\b\f\n\r\t")));
    }

    [Fact]
    public void stringifyUnsafeCharacters() {
        Assert.Equal("\"\\u0000\\u001B\"", JSON.stringify(new JSON.Value("\0\u001B")));
        Assert.Equal("\"\\u2028\\u2029\"", JSON.stringify(new JSON.Value("\u2028\u2029")));
    }

    [Fact]
    public void preserveUnicodeByDefault() {
        Assert.Equal("\"Привет 世界 👋\"", JSON.stringify(new JSON.Value("Привет 世界 👋")));
    }

    [Fact]
    public void escapeNonASCII() {
        JSON.StringifyOptions options = new();
        options.escapeNonASCII = true;

        Assert.Equal("\"\\u0416\\u4E2D\\uD83D\\uDC4B\"", JSON.stringify(new JSON.Value("Ж中👋"), options));
    }

    [Fact]
    public void stringifyEmptyContainers() {
        Assert.Equal("[]", JSON.stringify(new JSON.Value(new JSON.Array())));
        Assert.Equal("{}", JSON.stringify(new JSON.Value(new JSON.Object())));
    }

    [Fact]
    public void stringifyPrettyArray() {
        JSON.Array array = [1, "two", true, new JSON.Value()];

        Assert.Equal(
            "[\n" +
            "    1,\n" +
            "    \"two\",\n" +
            "    true,\n" +
            "    null\n" +
            "]",
            JSON.stringify(new JSON.Value(array)));
    }

    [Fact]
    public void stringifyCompactArray() {
        JSON.StringifyOptions options = new();
        options.pretty = false;
        
        JSON.Array array = [1, "two", true, new JSON.Value()];
        Assert.Equal("[1,\"two\",true,null]", JSON.stringify(new JSON.Value(array), options));
    }

    [Fact]
    public void stringifyPrettyObject() {
        JSON.Object obj = new();
        obj.Add("name", "Neoluma");
        obj.Add("version", 1);
        obj.Add("enabled", true);

        Assert.Equal(
            "{\n" +
            "    \"name\": \"Neoluma\",\n" +
            "    \"version\": 1,\n" +
            "    \"enabled\": true\n" +
            "}",
            JSON.stringify(new JSON.Value(obj)));
    }

    [Fact]
    public void stringifyCompactObject() {
        JSON.StringifyOptions options = new();
        options.pretty = false;

        JSON.Object obj = new();
        obj.Add("name", "Neoluma");
        obj.Add("version", 1);

        Assert.Equal("{\"name\":\"Neoluma\",\"version\":1}", JSON.stringify(new JSON.Value(obj), options));
    }

    [Fact]
    public void stringifyNestedValues() {
        JSON.Object project = new();
        project.Add("name", "Neoluma");
        project.Add("versions", new JSON.Array { 1, 2, 3 });

        JSON.Object root = new();
        root.Add("project", project);

        Assert.Equal(
            "{\n" +
            "    \"project\": {\n" +
            "        \"name\": \"Neoluma\",\n" +
            "        \"versions\": [\n" +
            "            1,\n" +
            "            2,\n" +
            "            3\n" +
            "        ]\n" +
            "    }\n" +
            "}",
            JSON.stringify(new JSON.Value(root)));
    }

    [Fact]
    public void customIndent() {
        JSON.StringifyOptions options = new();
        options.indent = 2;

        JSON.Array array = [new JSON.Array { 1, 2 }];

        Assert.Equal(
            "[\n" +
            "  [\n" +
            "    1,\n" +
            "    2\n" +
            "  ]\n" +
            "]",
            JSON.stringify(new JSON.Value(array), options));
    }

    [Fact]
    public void preserveObjectKeyOrderByDefault() {
        JSON.StringifyOptions options = new();
        options.pretty = false;

        JSON.Object obj = new();
        obj.Add("z", 1);
        obj.Add("a", 2);
        obj.Add("m", 3);

        Assert.Equal("{\"z\":1,\"a\":2,\"m\":3}", JSON.stringify(new JSON.Value(obj), options));
    }

    [Fact]
    public void sortObjectKeys() {
        JSON.StringifyOptions options = new();
        options.pretty = false;
        options.sortKeys = true;

        JSON.Object obj = new();
        obj.Add("z", 1);
        obj.Add("a", 2);
        obj.Add("m", 3);

        Assert.Equal("{\"a\":2,\"m\":3,\"z\":1}", JSON.stringify(new JSON.Value(obj), options));
    }

    [Fact]
    public void stringifyRootComments() {
        JSON.Value value = 1;

        value.commentsBefore.Add(" before");
        value.commentsAfter.Add(" after");

        Assert.Equal(
            "// before\n" +
            "1\n" +
            "// after",
            JSON.stringify(value));
    }

    [Fact]
    public void stringifyMultilineComment() {
        JSON.Value value = 1;

        value.commentsBefore.Add(" first line\nsecond line ");

        Assert.Equal(
            "/* first line\nsecond line */\n" +
            "1",
            JSON.stringify(value));
    }

    [Fact]
    public void disableComments() {
        JSON.StringifyOptions options = new();
        options.emitComments = false;

        JSON.Value value = 1;
        value.commentsBefore.Add(" before");
        value.commentsAfter.Add(" after");

        Assert.Equal("1", JSON.stringify(value, options));
    }

    [Fact]
    public void stringifyArrayCommentsBefore() {
        JSON.Array array = [1, 2];

        array[0].commentsBefore.Add(" first");
        array[1].commentsBefore.Add(" second");

        Assert.Equal(
            "[\n" +
            "    // first\n" +
            "    1,\n" +
            "    // second\n" +
            "    2\n" +
            "]",
            JSON.stringify(new JSON.Value(array)));
    }

    [Fact]
    public void stringifyArrayCommentsAfter() {
        JSON.Array array = [1, 2];

        array[0].commentsAfter.Add(" first");

        Assert.Equal(
            "[\n" +
            "    1, // first\n" +
            "    2\n" +
            "]",
            JSON.stringify(new JSON.Value(array)));
    }

    [Fact]
    public void stringifyObjectComments() {
        JSON.Object obj = new();
        obj.Add("a", 1);
        obj.Add("b", 2);

        obj[0].value.commentsBefore.Add(" before a");
        obj[0].value.commentsAfter.Add(" after a");
        obj[1].value.commentsBefore.Add(" before b");

        Assert.Equal(
            "{\n" +
            "    // before a\n" +
            "    \"a\": 1, // after a\n" +
            "    // before b\n" +
            "    \"b\": 2\n" +
            "}",
            JSON.stringify(new JSON.Value(obj)));
    }

    [Fact]
    public void stringifyCompactWithComment() {
        JSON.StringifyOptions options = new();
        options.pretty = false;

        JSON.Array array = [1, 2];
        array[0].commentsAfter.Add(" after");

        Assert.Equal("[1, // after\n2]", JSON.stringify(new JSON.Value(array), options));
    }

    [Fact]
    public void stringifyBlockCommentAfterValue() {
        JSON.Array array = [1, 2];

        array[0].commentsAfter.Add(" first\ncontinued ");
        
        Assert.Equal(
            "[\n" +
            "    1, /* first\ncontinued */\n" +
            "    2\n" +
            "]",
            JSON.stringify(new JSON.Value(array)));
    }

    [Fact]
    public void roundtripPrimitiveValues() {
        JSON.Value[] values = [
            new JSON.Value(),
            new JSON.Value(42),
            new JSON.Value(-42),
            new JSON.Value(1.5),
            new JSON.Value(true),
            new JSON.Value(false),
            new JSON.Value("hello\nworld"),
            new JSON.Value(double.NaN),
            new JSON.Value(double.PositiveInfinity),
            new JSON.Value(double.NegativeInfinity)
        ];

        foreach (JSON.Value value in values) {
            JSON.Value parsed = JSON.parse(JSON.stringify(value));

            if (value.isNull()) Assert.True(parsed.isNull());
            else if (value.isInt()) Assert.Equal(value.asInt(), parsed.asInt());
            else if (value.isDouble()) {
                if (double.IsNaN(value.asDouble())) Assert.True(double.IsNaN(parsed.asDouble()));
                else Assert.Equal(value.asDouble(), parsed.asDouble());
            }
            else if (value.isBool()) Assert.Equal(value.asBool(), parsed.asBool());
            else if (value.isString()) Assert.Equal(value.asString(), parsed.asString());
        }
    }

    [Fact]
    public void roundtripNestedValue() {
        JSON.Object project = new();
        project.Add("name", "Neoluma");
        project.Add("enabled", true);
        project.Add("numbers", new JSON.Array { 1, 2.5, -3 });

        JSON.Value original = new JSON.Value(project);
        JSON.Value parsed = JSON.parse(JSON.stringify(original));

        Assert.Equal("Neoluma", parsed["name"].asString());
        Assert.True(parsed["enabled"].asBool());

        Assert.Equal(1L, parsed["numbers"][0].asInt());
        Assert.Equal(2.5, parsed["numbers"][1].asDouble());
        Assert.Equal(-3L, parsed["numbers"][2].asInt());
    }

    [Fact]
    public void roundtripComments() {
        JSON.Array array = [1, 2];

        array[0].commentsBefore.Add(" before first");
        array[0].commentsAfter.Add(" after first");
        array[1].commentsBefore.Add(" before second");

        JSON.Value original = new JSON.Value(array);
        JSON.Value parsed = JSON.parse(JSON.stringify(original));

        Assert.Equal(" before first", parsed[0].commentsBefore[0]);
        Assert.Equal(" after first", parsed[0].commentsAfter[0]);
        Assert.Equal(" before second", parsed[1].commentsBefore[0]);
    }
}