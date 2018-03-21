// <copyright company="Soup">
//        Copyright (c) Soup.  All rights reserved.
// </copyright>

using Soup.Api;
using Soup.TestUtils;
using System;
using System.Threading.Tasks;
using Xunit;

namespace Soup.Client.UnitTests
{
	public class InitializeCommandTests : IDisposable
	{
		public InitializeCommandTests()
		{
			Singleton<LocalUserConfig>.Instance = new LocalUserConfig();
			Singleton<ISoupApi>.Instance = new MockSoupApi();
		}

		public void Dispose()
		{
			Singleton<LocalUserConfig>.Instance = null;
			Singleton<ISoupApi>.Instance = null;
		}

		[Fact]
		public void NameIsCorrect()
		{
			var uut = new InitializeCommand();
			Assert.Equal("init", uut.Name);
		}

		[Fact]
		public async Task CallWithZeroArgsShouldInitializeAll()
		{
			var uut = new InitializeCommand();

			var args = new string[] { };
			await uut.InvokeAsync(args);
		}


	}
}
