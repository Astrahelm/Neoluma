using Neoluma.Libraries;
namespace Neoluma.Tests.Libraries;

public class LexerTest {
    [Fact]
    public void validateIntegers() {
        Assert.Equal(0L, JSON.parse("0").asInt());
        Assert.Equal(42L, JSON.parse("42").asInt());
        Assert.Equal(-42L, JSON.parse("-42").asInt());
        Assert.Equal(42L, JSON.parse("+42").asInt());
    }

    [Fact]
    public void validateHexadecimalNumbers() {
        Assert.Equal(16L, JSON.parse("0x10").asInt());
        Assert.Equal(255L, JSON.parse("0xFF").asInt());
        Assert.Equal(-16L, JSON.parse("-0x10").asInt());
        Assert.Equal(16L, JSON.parse("+0X10").asInt());
    }

    [Fact]
    public void validateDoubles() {
        Assert.Equal(1.5, JSON.parse("1.5").asDouble());
        Assert.Equal(0.5, JSON.parse(".5").asDouble());
        Assert.Equal(5.0, JSON.parse("5.").asDouble());
        Assert.Equal(1000.0, JSON.parse("1e3").asDouble());
        Assert.Equal(0.001, JSON.parse("1e-3").asDouble());
    }

    [Fact]
    public void validateSpecialNumbers() {
        Assert.True(double.IsPositiveInfinity(JSON.parse("Infinity").asDouble()));
        Assert.True(double.IsNegativeInfinity(JSON.parse("-Infinity").asDouble()));
        Assert.True(double.IsNaN(JSON.parse("NaN").asDouble()));
    }

    [Fact]
    public void validateStrings() {
        Assert.Equal("test", JSON.parse("\"test\"").asString());
        Assert.Equal("test", JSON.parse("'test'").asString());
    }

    [Fact]
    public void validateIdentifiers() {
        JSON.Value root = JSON.parse("{ test: 1, _key: 2, $key: 3 }");

        Assert.Equal(1L, root["test"].asInt());
        Assert.Equal(2L, root["_key"].asInt());
        Assert.Equal(3L, root["$key"].asInt());
    }

    [Fact]
    public void validateComments() {
        JSON.Value root = JSON.parse("// before\n1 // after");

        Assert.Single(root.commentsBefore);
        Assert.Single(root.commentsAfter);

        Assert.Equal(" before", root.commentsBefore[0]);
        Assert.Equal(" after", root.commentsAfter[0]);
    }

    [Fact]
    public void rejectInvalidNumbers() {
        Assert.Throws<JSON.ParseError>(() => JSON.parse("."));
        Assert.Throws<JSON.ParseError>(() => JSON.parse("+"));
        Assert.Throws<JSON.ParseError>(() => JSON.parse("-"));
        Assert.Throws<JSON.ParseError>(() => JSON.parse("0x"));
        Assert.Throws<JSON.ParseError>(() => JSON.parse("1e"));
    }

    [Fact]
    public void rejectInvalidStrings() {
        Assert.Throws<JSON.ParseError>(() => JSON.parse("\"test"));
        Assert.Throws<JSON.ParseError>(() => JSON.parse("'test"));
    }

    [Fact]
    public void rejectUnterminatedComments() {
        Assert.Throws<JSON.ParseError>(() => JSON.parse("/* test"));
    }

    [Fact]
    public void validateSingleQuoteOption() {
        JSON.ParseOptions options = new();
        options.allowSingleQuotes = false;

        Assert.Throws<JSON.ParseError>(() => JSON.parse("'test'", options));
    }
    
    [Fact]
    public void decodeStringEscapes() {
        Assert.Equal("line\nbreak", JSON.parse("\"line\\nbreak\"").asString());
        Assert.Equal("tab\there", JSON.parse("\"tab\\there\"").asString());
        Assert.Equal("\"quoted\"", JSON.parse("\"\\\"quoted\\\"\"").asString());
        Assert.Equal("\\", JSON.parse("\"\\\\\"").asString());

        Assert.Equal("A", JSON.parse("\"\\x41\"").asString());
        Assert.Equal("A", JSON.parse("\"\\u0041\"").asString());
        Assert.Equal("🎼", JSON.parse("\"\\uD83C\\uDFBC\"").asString());

        Assert.Equal("AC/DC", JSON.parse("\"\\A\\C\\/\\D\\C\"").asString());
    }

    [Fact]
    public void rejectInvalidStringEscapes() {
        Assert.Throws<JSON.ParseError>(() => JSON.parse("\"\\x4\""));
        Assert.Throws<JSON.ParseError>(() => JSON.parse("\"\\u123\""));
        Assert.Throws<JSON.ParseError>(() => JSON.parse("\"\\1\""));
    }
}