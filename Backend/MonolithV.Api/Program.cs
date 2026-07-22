var builder = WebApplication.CreateBuilder(args);

// Add services to the container.

builder.Services.AddControllers();
// Learn more about configuring OpenAPI at https://aka.ms/aspnet/openapi
// Register Oracle Data Access Layer (P1.8) & Redis Cache-Aside Decorator (P1.9)
builder.Services.AddSingleton<StackExchange.Redis.IConnectionMultiplexer>(sp =>
{
    var configuration = sp.GetRequiredService<IConfiguration>();
    var redisConnectionString = configuration["Redis:ConnectionString"] ?? "localhost:6379";
    return StackExchange.Redis.ConnectionMultiplexer.Connect(redisConnectionString);
});
builder.Services.AddScoped<MonolithV.Data.IOracleConnectionFactory, MonolithV.Data.OracleConnectionFactory>();
builder.Services.AddScoped<MonolithV.Data.PlayerRepository>();
builder.Services.AddScoped<MonolithV.Data.IPlayerRepository>(sp =>
    new MonolithV.Data.CachedPlayerRepository(
        sp.GetRequiredService<MonolithV.Data.PlayerRepository>(),
        sp.GetRequiredService<StackExchange.Redis.IConnectionMultiplexer>(),
        sp.GetRequiredService<ILogger<MonolithV.Data.CachedPlayerRepository>>()
    ));
builder.Services.AddScoped<MonolithV.Data.IShareEventRepository, MonolithV.Data.ShareEventRepository>();
builder.Services.AddScoped<MonolithV.Data.ICheckpointRepository, MonolithV.Data.CheckpointRepository>();

var app = builder.Build();

// Configure the HTTP request pipeline.
if (app.Environment.IsDevelopment())
{
    app.MapOpenApi();
}

app.UseHttpsRedirection();

app.UseAuthorization();

app.MapControllers();

app.Run();
