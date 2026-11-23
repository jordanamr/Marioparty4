using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using System.Text.Json;

// The shared lobby expects the .NET Framework serializer's untyped object
// tree: dictionary values use ArrayList, an untyped root array uses object[], and
// integer fields such as seats and mod IDs arrive as int.
namespace System.Web.Script.Serialization {
sealed class JavaScriptSerializer {
    public int MaxJsonLength {get; set;} = 2 * 1024 * 1024;

    public string Serialize(object value) => JsonSerializer.Serialize(value);
    public T Deserialize<T>(string json) {
        if(json.Length>MaxJsonLength) throw new JsonException("JSON exceeds the size limit.");
        using(var document=JsonDocument.Parse(json))
            return (T)ConvertElement(document.RootElement,typeof(T)==typeof(object));
    }

    static object ConvertElement(JsonElement element,bool rootArray=false) {
        switch(element.ValueKind) {
            case JsonValueKind.Object:
                var fields=new Dictionary<string,object>(StringComparer.Ordinal);
                foreach(var field in element.EnumerateObject())fields[field.Name]=ConvertElement(field.Value);
                return fields;
            case JsonValueKind.Array:
                var items=element.EnumerateArray().Select(item=>ConvertElement(item)).ToArray();
                return rootArray?items:(object)new ArrayList(items);
            case JsonValueKind.String: return element.GetString();
            case JsonValueKind.Number:
                if(element.TryGetInt32(out var integer))return integer;
                if(element.TryGetInt64(out var wide))return wide;
                return element.GetDouble();
            case JsonValueKind.True: return true;
            case JsonValueKind.False: return false;
            case JsonValueKind.Null: return null;
            default: throw new JsonException();
        }
    }
}
}
