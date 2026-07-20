using Neoluma.Libraries;

namespace Neoluma.Tests.Libraries;

public class ValueTest {

    [Fact]
    public void validateConstructors() {
        Assert.IsType<long>(new JSON.Value(1).raw());
        Assert.IsType<double>(new JSON.Value(1.5).raw());
        Assert.IsType<string>(new JSON.Value("test").raw());
        Assert.IsType<bool>(new JSON.Value(false).raw());
        Assert.IsType<JSON.Array>(new JSON.Value([0, 1, 2]).raw());

        JSON.Property prop = new JSON.Property("test", new JSON.Value(1));
        JSON.Object obj = new JSON.Object(); obj.Add(prop);
        Assert.IsType<JSON.Object>(new JSON.Value(obj).raw());
    }

    [Fact]
    public void validateTypeChecks() {
        Assert.True(new JSON.Value(1).isInt());
        Assert.True(new JSON.Value(1.5).isDouble());
        Assert.True(new JSON.Value("test").isString());
        Assert.True(new JSON.Value(false).isBool());
        Assert.True(new JSON.Value([1]).isArray());
        Assert.True(new JSON.Value(new JSON.Object()).isObject());
    }
    
    [Fact]
    public void validate
}