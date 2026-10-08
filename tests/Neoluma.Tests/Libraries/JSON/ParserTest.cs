using Neoluma.Libraries;
namespace Neoluma.Tests.Libraries;

public class ParserTest {
    [Fact]
    public void parsePrimitiveValues() {
        Assert.True(JSON.parse("null").isNull());
        Assert.True(JSON.parse("true").asBool());
        Assert.False(JSON.parse("false").asBool());
        Assert.Equal(67L, JSON.parse("67").asInt());
        Assert.Equal(6.9, JSON.parse("6.9").asDouble());
        Assert.Equal("test", JSON.parse("\"test\"").asString());
    }

    [Fact]
    public void parseArray() {
        JSON.Value root = JSON.parse("[1, \"two\", true, null]");

        Assert.True(root.isArray());
        Assert.Equal(4, root.asArray().Count);

        Assert.Equal(1L, root[0].asInt());
        Assert.Equal("two", root[1].asString());
        Assert.True(root[2].asBool());
        Assert.True(root[3].isNull());
    }

    [Fact]
    public void parseObject() {
        JSON.Value root = JSON.parse("""
        {
            name: "Neoluma",
            version: 1,
            enabled: true
        }
        """);

        Assert.True(root.isObject());

        Assert.Equal("Neoluma", root["name"].asString());
        Assert.Equal(1L, root["version"].asInt());
        Assert.True(root["enabled"].asBool());
    }

    [Fact]
    public void parseNestedValues() {
        JSON.Value root = JSON.parse("""
        {
            project: {
                name: "Neoluma",
                versions: [1, 2, 3]
            }
        }
        """);

        Assert.Equal("Neoluma", root["project"]["name"].asString());
        Assert.Equal(1L, root["project"]["versions"][0].asInt());
        Assert.Equal(2L, root["project"]["versions"][1].asInt());
        Assert.Equal(3L, root["project"]["versions"][2].asInt());
    }

    [Fact]
    public void parseTrailingCommas() {
        JSON.Value array = JSON.parse("[1, 2, 3,]");
        JSON.Value obj = JSON.parse("{ a: 1, b: 2, }");

        Assert.Equal(3, array.asArray().Count);
        Assert.Equal(2, obj.asObject().Count);
    }

    [Fact]
    public void rejectTrailingCommasWhenDisabled() {
        JSON.ParseOptions options = new();
        options.allowTrailingCommas = false;

        Assert.Throws<JSON.ParseError>(() => JSON.parse("[1,]", options));
        Assert.Throws<JSON.ParseError>(() => JSON.parse("{ a: 1, }", options));
    }

    [Fact]
    public void replaceDuplicateKeys() {
        JSON.Value root = JSON.parse("{ value: 1, value: 2 }");

        Assert.Equal(2L, root["value"].asInt());
        Assert.Single(root.asObject());
    }

    [Fact]
    public void rejectDuplicateKeysWhenDisabled() {
        JSON.ParseOptions options = new();
        options.duplicateKeysLastWins = false;

        Assert.Throws<JSON.ParseError>(
            () => JSON.parse("{ value: 1, value: 2 }", options)
        );
    }

    [Fact]
    public void parseComments() {
        JSON.Value root = JSON.parse("""
        // root before
        [
            // first before
            1 /* first after */,
            // second before
            2
        ]
        // root after
        """);

        Assert.Equal(" root before", root.commentsBefore[0]);
        Assert.Equal(" root after", root.commentsAfter[0]);

        Assert.Equal(" first before", root[0].commentsBefore[0]);
        Assert.Equal(" first after ", root[0].commentsAfter[0]);
        Assert.Equal(" second before", root[1].commentsBefore[0]);
    }

    [Fact]
    public void parseCommentAfterComma() {
        JSON.Value root = JSON.parse("""
        [
            1, /* first after */
            2
        ]
        """);

        Assert.Equal(" first after ", root[0].commentsAfter[0]);
    }

    [Fact]
    public void rejectCommentsWhenDisabled() {
        JSON.ParseOptions options = new();
        options.allowComments = false;

        Assert.Throws<JSON.ParseError>(
            () => JSON.parse("// comment\n1", options)
        );
    }

    [Fact]
    public void rejectIdentifierAsValue() {
        Assert.Throws<JSON.ParseError>(
            () => JSON.parse("{ key: value }")
        );
    }

    [Fact]
    public void rejectUnclosedContainers() {
        Assert.Throws<JSON.ParseError>(() => JSON.parse("[1, 2"));
        Assert.Throws<JSON.ParseError>(() => JSON.parse("{ a: 1"));
    }

    [Fact]
    public void rejectMissingCommas() {
        Assert.Throws<JSON.ParseError>(() => JSON.parse("[1 2]"));
        Assert.Throws<JSON.ParseError>(() => JSON.parse("{ a: 1 b: 2 }"));
    }

    [Fact]
    public void rejectTrailingValues() {
        Assert.Throws<JSON.ParseError>(() => JSON.parse("1 2"));
        Assert.Throws<JSON.ParseError>(() => JSON.parse("{} []"));
    }
}