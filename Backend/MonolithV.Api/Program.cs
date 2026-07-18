var builder = WebApplication.CreateBuilder(args);

// Add services to the container.

builder.Services.AddControllers();
// Learn more about configuring OpenAPI at https://aka.ms/aspnet/openapi
// Register Oracle Data Access Layer (P1.8)
builder.Services.AddScoped<MonolithV.Data.IOracleConnectionFactory, MonolithV.Data.OracleConnectionFactory>();
builder.Services.AddScoped<MonolithV.Data.IPlayerRepository, MonolithV.Data.PlayerRepository>();

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
