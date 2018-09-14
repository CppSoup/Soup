﻿

using System.Collections.Generic;
using System.Linq;

namespace Soup.StaticAnalysis.AST
{
    /// <summary>
    /// Declaration sequence
    /// </summary>
    public sealed class DeclarationSequence : Node
    {
        /// <summary>
        /// Gets or sets the list of declarations
        /// </summary>
        public IList<Declaration> Declarations { get; set; } = new List<Declaration>();

        /// <summary>
        /// Equals
        /// </summary>
        public override bool Equals(object obj)
        {
            var other = obj as DeclarationSequence;
            if (other == null)
            {
                return false;
            }

            return Declarations.SequenceEqual(other.Declarations);
        }

        /// <summary>
        /// Get hash code
        /// </summary>
        public override int GetHashCode()
        {
            return Declarations.GetHashCode();
        }
    }
}
