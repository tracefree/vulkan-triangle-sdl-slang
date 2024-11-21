static const float2 positions[3] = {
    float2(0.0, -0.5),
    float2(0.5, 0.5),
    float2(-0.5, 0.5)
};

static const float3 colors[3] = {
    float3(1.0, 0.0, 0.0),
    float3(0.0, 1.0, 0.0),
    float3(0.0, 0.0, 1.0)
};

struct VSOutput {
    float4 PositionCS : SV_Position;
    float3 Color : VertexColor;
};

[shader("vertex")]
VSOutput vertex(uint vertexID: SV_VertexID) {
    return VSOutput(
        float4(positions[vertexID], 1.0),
        colors[vertexID]
    );
}

[shader("fragment")]
float4 fragment(float3 color: VertexColor): SV_Target {
    return float4(color.rgb, 1.0);
}