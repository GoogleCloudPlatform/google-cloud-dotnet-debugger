FROM gcr.io/dotnet-debugger/aspnetcore:2.0
COPY . /app
WORKDIR /app
COPY ./source-context.json /usr/share/dotnet-debugger/agent/
ENTRYPOINT ["/usr/share/dotnet-debugger/start-debugger.sh", "dotnet", "/app/Google.Cloud.Diagnostics.Debug.TestApp.dll"]