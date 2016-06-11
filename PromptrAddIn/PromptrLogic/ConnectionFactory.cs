using System;
using System.Collections.Generic;
using PromptrLib;
using Q42.HueApi.NET;
using Q42.HueApi;
using Q42.HueApi.Interfaces;

namespace PromptrLib
{
    public class ConnectionFactory
    {
        private readonly string appKey;

        public ConnectionFactory(string appKey)
        {
            this.appKey = appKey;
        }

        public Connection GetConnection()
        {
            ILocalHueClient client = new LocalHueClient("10.0.1.2");
            client.Initialize(appKey);

            return new Connection(client);
        }

        
    }
}
