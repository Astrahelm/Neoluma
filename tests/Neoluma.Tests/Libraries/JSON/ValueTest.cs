using Neoluma.Libraries;
namespace Neoluma.Tests.Libraries;

public class ValueTest {
    [Fact]
    public void validateConstructors() {
        Assert.Null(new JSON.Value().raw());
        Assert.IsType<long>(new JSON.Value(1).raw());
        Assert.IsType<double>(new JSON.Value(1.5).raw());
        Assert.IsType<string>(new JSON.Value("test").raw());
        Assert.IsType<bool>(new JSON.Value(false).raw());
        Assert.IsType<JSON.Array>(new JSON.Value([0, 1, 2]).raw());

        JSON.Property prop = new JSON.Property("test", new JSON.Value(1));
        JSON.Object obj = new(); 
        obj.Add(prop);

        Assert.IsType<JSON.Object>(new JSON.Value(obj).raw());
    }

    [Fact]
    public void validateTypeChecks() {
        Assert.True(new JSON.Value().isNull());
        Assert.True(new JSON.Value(1).isInt());
        Assert.True(new JSON.Value(1.5).isDouble());
        Assert.True(new JSON.Value("test").isString());
        Assert.True(new JSON.Value(false).isBool());
        Assert.True(new JSON.Value([1]).isArray());
        Assert.True(new JSON.Value(new JSON.Object()).isObject());
    }

    [Fact]
    public void validateGetters() {
        Assert.Equal(5L, new JSON.Value(5).asInt());
        Assert.Equal(5.5, new JSON.Value(5.5).asDouble());
        Assert.Equal(5.0, new JSON.Value(5).asDouble());
        Assert.Equal("test", new JSON.Value("test").asString());
        Assert.True(new JSON.Value(true).asBool());

        JSON.Array arr = [1, 2, 3];

        JSON.Object obj = new();
        obj.Add("test", 1);

        Assert.Same(arr, new JSON.Value(arr).asArray());
        Assert.Same(obj, new JSON.Value(obj).asObject());
    }

    [Fact]
    public void validateIndexing() {
        JSON.Value obj = new JSON.Value(new JSON.Object());

        obj["name"] = "Neoluma";
        obj["version"] = 1;

        Assert.Equal("Neoluma", obj["name"].asString());
        Assert.Equal(1L, obj["version"].asInt());

        JSON.Value arr = new JSON.Value(new JSON.Array());

        arr[2] = "third";

        Assert.True(arr[0].isNull());
        Assert.True(arr[1].isNull());
        Assert.Equal("third", arr[2].asString());
    }

    [Fact]
    public void validateImplicitConversions() {
        JSON.Value integer = 1;
        JSON.Value number = 1.5;
        JSON.Value text = "test";
        JSON.Value boolean = true;

        JSON.Array arr = [1, "two", false];
        JSON.Value array = arr;

        Assert.Equal(1L, integer.asInt());
        Assert.Equal(1.5, number.asDouble());
        Assert.Equal("test", text.asString());
        Assert.True(boolean.asBool());
        Assert.Same(arr, array.asArray());
    }
}