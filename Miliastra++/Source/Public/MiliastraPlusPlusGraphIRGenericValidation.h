#pragma once

#include <algorithm>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "MiliastraPlusPlusDescriptors.h"
#include "MiliastraPlusPlusGraphIR.h"

namespace MiliastraPlusPlus::GraphIRGenericValidationDetail
{
    enum class GenericOwnerKind : std::uint8_t
    {
        NodeInstance,
        GraphVariable
    };

    struct GenericOwnerScope
    {
        GenericOwnerKind Kind = GenericOwnerKind::NodeInstance;
        NodeInstanceId Node;
        GraphVariableId Variable;

        auto operator<=>(const GenericOwnerScope&) const = default;

        [[nodiscard]] static GenericOwnerScope ForNode(NodeInstanceId Identifier)
        {
            return {GenericOwnerKind::NodeInstance, Identifier, {}};
        }

        [[nodiscard]] static GenericOwnerScope ForVariable(GraphVariableId Identifier)
        {
            return {GenericOwnerKind::GraphVariable, {}, Identifier};
        }
    };

    struct GenericConstraintKey
    {
        GenericOwnerScope Owner;
        GenericParameterId Parameter;

        auto operator<=>(const GenericConstraintKey&) const = default;
    };

    struct TypeRelation
    {
        TypeDesc Left;
        GenericOwnerScope LeftScope;
        TypeDesc Right;
        GenericOwnerScope RightScope;
    };

    struct Conflict
    {
        GenericConstraintKey Key;
        TypeDesc Left;
        TypeDesc Right;

        auto operator<=>(const Conflict&) const = default;
    };

    struct Binding
    {
        TypeDesc Type;
        GenericOwnerScope Scope;
    };

    class Environment
    {
    public:
        explicit Environment(std::vector<GenericConstraintKey> Keys)
            : m_Keys(std::move(Keys))
            , m_Parents(m_Keys.size())
            , m_Bindings(m_Keys.size())
        {
            // Solver::Run supplies sorted, unique keys; this order defines representatives.
            for (std::size_t Index = 0U; Index < m_Parents.size(); ++Index)
            {
                m_Parents[Index] = Index;
            }
        }

        [[nodiscard]] bool Contains(const GenericConstraintKey& Key) const
        {
            return FindIndex(Key) != m_Keys.size();
        }

        [[nodiscard]] std::size_t FindIndex(const GenericConstraintKey& Key) const
        {
            const auto Iterator = std::lower_bound(
                m_Keys.begin(),
                m_Keys.end(),
                Key
            );
            if (Iterator == m_Keys.end() || *Iterator != Key)
            {
                return m_Keys.size();
            }
            return static_cast<std::size_t>(Iterator - m_Keys.begin());
        }

        [[nodiscard]] std::size_t FindRoot(std::size_t Index) const
        {
            if (Index >= m_Parents.size())
            {
                return m_Parents.size();
            }
            while (m_Parents[Index] != Index)
            {
                Index = m_Parents[Index];
            }
            return Index;
        }

        [[nodiscard]] std::size_t FindRoot(const GenericConstraintKey& Key) const
        {
            return FindRoot(FindIndex(Key));
        }

        [[nodiscard]] const GenericConstraintKey* GetKey(std::size_t Index) const
        {
            return Index < m_Keys.size() ? &m_Keys[Index] : nullptr;
        }

        [[nodiscard]] const std::optional<Binding>& GetBinding(std::size_t Index) const
        {
            const std::size_t Root = FindRoot(Index);
            return Root < m_Bindings.size() ? m_Bindings[Root] : EmptyBinding();
        }

        [[nodiscard]] const std::optional<Binding>& GetBinding(
            const GenericConstraintKey& Key
        ) const
        {
            return GetBinding(FindIndex(Key));
        }

        [[nodiscard]] std::size_t Union(
            const GenericConstraintKey& Left,
            const GenericConstraintKey& Right
        )
        {
            std::size_t LeftRoot = FindRoot(Left);
            std::size_t RightRoot = FindRoot(Right);
            if (LeftRoot >= m_Keys.size() || RightRoot >= m_Keys.size())
            {
                return m_Keys.size();
            }
            if (LeftRoot == RightRoot)
            {
                return LeftRoot;
            }

            if (m_Keys[RightRoot] < m_Keys[LeftRoot])
            {
                std::swap(LeftRoot, RightRoot);
            }

            m_Parents[RightRoot] = LeftRoot;
            if (!m_Bindings[LeftRoot].has_value() && m_Bindings[RightRoot].has_value())
            {
                m_Bindings[LeftRoot] = std::move(m_Bindings[RightRoot]);
            }
            return LeftRoot;
        }

        [[nodiscard]] bool SetBinding(
            const GenericConstraintKey& Key,
            Binding Value
        )
        {
            const std::size_t Root = FindRoot(Key);
            if (Root >= m_Bindings.size())
            {
                return false;
            }
            if (m_Bindings[Root].has_value())
            {
                return false;
            }
            m_Bindings[Root] = std::move(Value);
            return true;
        }

        [[nodiscard]] const std::vector<GenericConstraintKey>& GetKeys() const
        {
            return m_Keys;
        }

    private:
        [[nodiscard]] static const std::optional<Binding>& EmptyBinding()
        {
            static const std::optional<Binding> Empty;
            return Empty;
        }

        std::vector<GenericConstraintKey> m_Keys;
        std::vector<std::size_t> m_Parents;
        std::vector<std::optional<Binding>> m_Bindings;
    };

    [[nodiscard]] inline const NodeDescriptor* FindDescriptor(
        const NodeInstance* Node,
        const NodeDescriptorRegistry& Descriptors
    )
    {
        if (Node == nullptr || !Node->Descriptor.IsValid())
        {
            return nullptr;
        }
        return Descriptors.Find(Node->Descriptor);
    }

    [[nodiscard]] inline const PinSchema* FindPin(
        const NodeInstance* Node,
        PinIndex Index,
        const NodeDescriptorRegistry& Descriptors
    )
    {
        const NodeDescriptor* Descriptor = FindDescriptor(Node, Descriptors);
        if (Descriptor == nullptr || !Index.IsValid() ||
            Index.GetValue() >= Descriptor->GetPins().size())
        {
            return nullptr;
        }
        return &Descriptor->GetPins()[Index.GetValue()];
    }

    [[nodiscard]] inline GenericConstraintKey MakeKey(
        const GenericOwnerScope& Scope,
        GenericParameterId Parameter
    )
    {
        return {Scope, Parameter};
    }

    inline void CollectKeys(
        const TypeDesc& Type,
        const GenericOwnerScope& Scope,
        std::vector<GenericConstraintKey>& Keys
    )
    {
        if (!Type.IsValid())
        {
            return;
        }
        switch (Type.GetKind())
        {
        case TypeDesc::Kind::Generic:
            Keys.push_back(MakeKey(Scope, Type.GetGenericParameter()));
            return;
        case TypeDesc::Kind::List:
            CollectKeys(*Type.GetElementType(), Scope, Keys);
            return;
        case TypeDesc::Kind::Dictionary:
            CollectKeys(*Type.GetKeyType(), Scope, Keys);
            CollectKeys(*Type.GetValueType(), Scope, Keys);
            return;
        default:
            return;
        }
    }

    [[nodiscard]] inline bool ContainsGeneric(const TypeDesc& Type)
    {
        if (!Type.IsValid())
        {
            return false;
        }
        switch (Type.GetKind())
        {
        case TypeDesc::Kind::Generic:
            return true;
        case TypeDesc::Kind::List:
            return ContainsGeneric(*Type.GetElementType());
        case TypeDesc::Kind::Dictionary:
            return ContainsGeneric(*Type.GetKeyType()) ||
                ContainsGeneric(*Type.GetValueType());
        default:
            return false;
        }
    }

    [[nodiscard]] inline bool ContainsFlow(const TypeDesc& Type)
    {
        if (!Type.IsValid())
        {
            return false;
        }
        switch (Type.GetKind())
        {
        case TypeDesc::Kind::Flow:
            return true;
        case TypeDesc::Kind::List:
            return ContainsFlow(*Type.GetElementType());
        case TypeDesc::Kind::Dictionary:
            return ContainsFlow(*Type.GetKeyType()) ||
                ContainsFlow(*Type.GetValueType());
        default:
            return false;
        }
    }

    [[nodiscard]] inline std::optional<GenericConstraintKey> FirstGeneric(
        const TypeDesc& Type,
        const GenericOwnerScope& Scope
    )
    {
        if (!Type.IsValid())
        {
            return std::nullopt;
        }
        switch (Type.GetKind())
        {
        case TypeDesc::Kind::Generic:
            return MakeKey(Scope, Type.GetGenericParameter());
        case TypeDesc::Kind::List:
            return FirstGeneric(*Type.GetElementType(), Scope);
        case TypeDesc::Kind::Dictionary:
            if (const auto Key = FirstGeneric(*Type.GetKeyType(), Scope); Key.has_value())
            {
                return Key;
            }
            return FirstGeneric(*Type.GetValueType(), Scope);
        default:
            return std::nullopt;
        }
    }

    [[nodiscard]] inline bool IsConcreteCompatible(
        const TypeDesc& Left,
        const TypeDesc& Right
    )
    {
        return !ContainsGeneric(Left) && !ContainsGeneric(Right) &&
            Left.IsCompatibleWith(Right);
    }

    [[nodiscard]] inline std::string DescribeKey(const GenericConstraintKey& Key)
    {
        const char* OwnerName = Key.Owner.Kind == GenericOwnerKind::NodeInstance
            ? "node"
            : "graph variable";
        const std::uint32_t OwnerId = Key.Owner.Kind == GenericOwnerKind::NodeInstance
            ? Key.Owner.Node.GetValue()
            : Key.Owner.Variable.GetValue();
        return std::string(OwnerName) + " " + std::to_string(OwnerId) +
            ", parameter " + std::to_string(Key.Parameter.GetValue());
    }

    [[nodiscard]] inline std::string DescribeType(const TypeDesc& Type)
    {
        if (!Type.IsValid())
        {
            return "Invalid";
        }
        switch (Type.GetKind())
        {
        case TypeDesc::Kind::Invalid: return "Invalid";
        case TypeDesc::Kind::Boolean: return "Boolean";
        case TypeDesc::Kind::Integer: return "Integer";
        case TypeDesc::Kind::Float: return "Float";
        case TypeDesc::Kind::String: return "String";
        case TypeDesc::Kind::Flow: return "Flow";
        case TypeDesc::Kind::Entity: return "Entity";
        case TypeDesc::Kind::GUID: return "GUID";
        case TypeDesc::Kind::Vector3: return "Vector3";
        case TypeDesc::Kind::PrefabId: return "PrefabId";
        case TypeDesc::Kind::ConfigId: return "ConfigId";
        case TypeDesc::Kind::Faction: return "Faction";
        case TypeDesc::Kind::Generic:
            return "Generic<" + std::to_string(Type.GetGenericParameter().GetValue()) + ">";
        case TypeDesc::Kind::List:
            return "List<" + DescribeType(*Type.GetElementType()) + ">";
        case TypeDesc::Kind::Dictionary:
            return "Dictionary<" + DescribeType(*Type.GetKeyType()) + ", " +
                DescribeType(*Type.GetValueType()) + ">";
        case TypeDesc::Kind::StructObject:
            return "StructObject<" + std::to_string(Type.GetStructType().GetValue()) + ">";
        }
        return "Invalid";
    }

    class Solver
    {
    public:
        Solver(const GraphIR& Graph, const NodeDescriptorRegistry& Descriptors)
            : m_Graph(Graph)
            , m_Descriptors(Descriptors)
        {
        }

        [[nodiscard]] DiagnosticCollection Run()
        {
            std::vector<GenericConstraintKey> Keys;
            std::vector<TypeRelation> Relations;
            CollectGraphTypes(Keys);
            CollectRelations(Keys, Relations);

            std::sort(Keys.begin(), Keys.end());
            Keys.erase(std::unique(Keys.begin(), Keys.end()), Keys.end());
            std::sort(Relations.begin(), Relations.end(), RelationLess);

            Environment EnvironmentValue(std::move(Keys));
            m_Environment = &EnvironmentValue;
            for (const TypeRelation& Relation : Relations)
            {
                Relate(
                    Relation.Left,
                    Relation.LeftScope,
                    Relation.Right,
                    Relation.RightScope,
                    std::nullopt
                );
            }

            ValidateResolvedTypes();
            DiagnosticCollection Diagnostics = BuildDiagnostics();
            m_Environment = nullptr;
            return Diagnostics;
        }

    private:
        static bool RelationLess(const TypeRelation& Left, const TypeRelation& Right)
        {
            if (const auto Comparison = Left.LeftScope <=> Right.LeftScope; Comparison != 0)
            {
                return Comparison < 0;
            }
            if (const auto Comparison = Left.Left <=> Right.Left; Comparison != 0)
            {
                return Comparison < 0;
            }
            if (const auto Comparison = Left.RightScope <=> Right.RightScope; Comparison != 0)
            {
                return Comparison < 0;
            }
            return (Left.Right <=> Right.Right) < 0;
        }

        void CollectGraphTypes(std::vector<GenericConstraintKey>& Keys) const
        {
            for (const NodeInstance& Node : m_Graph.GetNodes())
            {
                const NodeDescriptor* Descriptor = FindDescriptor(&Node, m_Descriptors);
                if (Descriptor == nullptr)
                {
                    continue;
                }
                const GenericOwnerScope Scope = GenericOwnerScope::ForNode(Node.Identifier);
                for (const PinSchema& Pin : Descriptor->GetPins())
                {
                    CollectKeys(Pin.GetType(), Scope, Keys);
                }
            }

            for (const GraphVariable& Variable : m_Graph.GetVariables())
            {
                CollectKeys(
                    Variable.Type,
                    GenericOwnerScope::ForVariable(Variable.Identifier),
                    Keys
                );
            }
        }

        void CollectRelations(
            std::vector<GenericConstraintKey>& Keys,
            std::vector<TypeRelation>& Relations
        ) const
        {
            for (const InputBindingRecord& Record : m_Graph.GetInputBindings())
            {
                const NodeInstance* DestinationNode = m_Graph.FindNode(Record.DestinationNode);
                const PinSchema* DestinationPin = FindPin(
                    DestinationNode,
                    Record.DestinationInputPin,
                    m_Descriptors
                );
                if (DestinationNode == nullptr || DestinationPin == nullptr ||
                    DestinationPin->GetDirection() != PinDirection::Input ||
                    DestinationPin->GetCategory() != PinCategory::Data)
                {
                    continue;
                }

                if (const OutputReference* Output = std::get_if<OutputReference>(&Record.Binding))
                {
                    const NodeInstance* SourceNode = m_Graph.FindNode(Output->SourceNode);
                    const PinSchema* SourcePin = FindPin(
                        SourceNode,
                        Output->SourceOutputPin,
                        m_Descriptors
                    );
                    if (SourceNode == nullptr || SourcePin == nullptr ||
                        SourcePin->GetDirection() != PinDirection::Output ||
                        SourcePin->GetCategory() != PinCategory::Data ||
                        ContainsFlow(SourcePin->GetType()) ||
                        ContainsFlow(DestinationPin->GetType()))
                    {
                        continue;
                    }

                    AddRelation(
                        SourcePin->GetType(),
                        GenericOwnerScope::ForNode(SourceNode->Identifier),
                        DestinationPin->GetType(),
                        GenericOwnerScope::ForNode(DestinationNode->Identifier),
                        Keys,
                        Relations
                    );
                    if (Record.OutputTypeConstraint.has_value() &&
                        Record.OutputTypeConstraint->IsValid() &&
                        !ContainsGeneric(*Record.OutputTypeConstraint) &&
                        !ContainsFlow(*Record.OutputTypeConstraint) &&
                        SourcePin->GetType().IsCompatibleWith(
                            *Record.OutputTypeConstraint))
                    {
                        const GenericOwnerScope SourceScope =
                            GenericOwnerScope::ForNode(SourceNode->Identifier);
                        AddRelation(
                            SourcePin->GetType(),
                            SourceScope,
                            *Record.OutputTypeConstraint,
                            SourceScope,
                            Keys,
                            Relations
                        );
                    }
                }
                else if (const GraphVariableReference* Variable =
                    std::get_if<GraphVariableReference>(&Record.Binding))
                {
                    const GraphVariable* SourceVariable = m_Graph.FindVariable(Variable->Variable);
                    if (SourceVariable == nullptr ||
                        ContainsFlow(SourceVariable->Type) ||
                        ContainsFlow(DestinationPin->GetType()))
                    {
                        continue;
                    }
                    AddRelation(
                        SourceVariable->Type,
                        GenericOwnerScope::ForVariable(SourceVariable->Identifier),
                        DestinationPin->GetType(),
                        GenericOwnerScope::ForNode(DestinationNode->Identifier),
                        Keys,
                        Relations
                    );
                }
            }
        }

        static void AddRelation(
            const TypeDesc& Left,
            const GenericOwnerScope& LeftScope,
            const TypeDesc& Right,
            const GenericOwnerScope& RightScope,
            std::vector<GenericConstraintKey>& Keys,
            std::vector<TypeRelation>& Relations
        )
        {
            if (!ContainsGeneric(Left) && !ContainsGeneric(Right))
            {
                return;
            }
            CollectKeys(Left, LeftScope, Keys);
            CollectKeys(Right, RightScope, Keys);
            Relations.push_back(TypeRelation{Left, LeftScope, Right, RightScope});
        }

        [[nodiscard]] bool IsActiveKey(
            std::size_t Root,
            const std::vector<GenericConstraintKey>& Active
        ) const
        {
            return std::any_of(Active.begin(), Active.end(), [this, Root](const auto& Key)
            {
                return m_Environment->Contains(Key) &&
                    m_Environment->FindRoot(Key) == Root;
            });
        }

        [[nodiscard]] bool IsActiveBindingPair(
            std::size_t LeftRoot,
            std::size_t RightRoot
        ) const
        {
            if (RightRoot < LeftRoot)
            {
                std::swap(LeftRoot, RightRoot);
            }
            return std::any_of(
                m_ActiveBindingPairs.begin(),
                m_ActiveBindingPairs.end(),
                [this, LeftRoot, RightRoot](const auto& Pair)
                {
                    std::size_t ActiveLeftRoot = m_Environment->FindRoot(Pair.first);
                    std::size_t ActiveRightRoot = m_Environment->FindRoot(Pair.second);
                    if (ActiveRightRoot < ActiveLeftRoot)
                    {
                        std::swap(ActiveLeftRoot, ActiveRightRoot);
                    }
                    return ActiveLeftRoot != ActiveRightRoot &&
                        ActiveLeftRoot == LeftRoot && ActiveRightRoot == RightRoot;
                }
            );
        }

        [[nodiscard]] TypeDesc Resolve(
            const TypeDesc& Type,
            const GenericOwnerScope& Scope,
            std::vector<GenericConstraintKey>& Active
        ) const
        {
            if (!Type.IsValid())
            {
                return Type;
            }
            if (Type.GetKind() == TypeDesc::Kind::Generic)
            {
                const GenericConstraintKey Key = MakeKey(Scope, Type.GetGenericParameter());
                if (!m_Environment->Contains(Key))
                {
                    return Type;
                }
                const std::size_t Root = m_Environment->FindRoot(Key);
                const GenericConstraintKey* RootKey = m_Environment->GetKey(Root);
                if (RootKey == nullptr || IsActiveKey(Root, Active))
                {
                    return Type;
                }
                const auto& BindingValue = m_Environment->GetBinding(Root);
                if (!BindingValue.has_value())
                {
                    return Type;
                }
                Active.push_back(*RootKey);
                TypeDesc Result = Resolve(BindingValue->Type, BindingValue->Scope, Active);
                Active.pop_back();
                return Result;
            }
            if (Type.GetKind() == TypeDesc::Kind::List)
            {
                return TypeDesc::List(Resolve(*Type.GetElementType(), Scope, Active));
            }
            if (Type.GetKind() == TypeDesc::Kind::Dictionary)
            {
                return TypeDesc::Dictionary(
                    Resolve(*Type.GetKeyType(), Scope, Active),
                    Resolve(*Type.GetValueType(), Scope, Active)
                );
            }
            return Type;
        }

        void Relate(
            const TypeDesc& Left,
            const GenericOwnerScope& LeftScope,
            const TypeDesc& Right,
            const GenericOwnerScope& RightScope,
            std::optional<GenericConstraintKey> FallbackKey
        )
        {
            const auto LeftGeneric = FirstGeneric(Left, LeftScope);
            const auto RightGeneric = FirstGeneric(Right, RightScope);
            if (!FallbackKey.has_value())
            {
                FallbackKey = LeftGeneric.has_value() ? LeftGeneric : RightGeneric;
            }

            if (Left.GetKind() == TypeDesc::Kind::Generic &&
                Right.GetKind() == TypeDesc::Kind::Generic)
            {
                const GenericConstraintKey LeftKey = MakeKey(
                    LeftScope,
                    Left.GetGenericParameter()
                );
                const GenericConstraintKey RightKey = MakeKey(
                    RightScope,
                    Right.GetGenericParameter()
                );
                if (m_Environment->Contains(LeftKey) && m_Environment->Contains(RightKey))
                {
                    const std::size_t LeftRoot = m_Environment->FindRoot(LeftKey);
                    const std::size_t RightRoot = m_Environment->FindRoot(RightKey);
                    if (LeftRoot == RightRoot)
                    {
                        return;
                    }

                    const auto LeftBinding = m_Environment->GetBinding(LeftKey);
                    const auto RightBinding = m_Environment->GetBinding(RightKey);
                    if (LeftBinding.has_value() && RightBinding.has_value())
                    {
                        auto ActivePair = std::pair{LeftRoot, RightRoot};
                        if (ActivePair.second < ActivePair.first)
                        {
                            std::swap(ActivePair.first, ActivePair.second);
                        }
                        if (!IsActiveBindingPair(ActivePair.first, ActivePair.second))
                        {
                            const Binding LeftBindingValue = *LeftBinding;
                            const Binding RightBindingValue = *RightBinding;
                            m_ActiveBindingPairs.push_back(ActivePair);
                            // Nested generic parameters retain the scope of their binding.
                            Relate(
                                LeftBindingValue.Type,
                                LeftBindingValue.Scope,
                                RightBindingValue.Type,
                                RightBindingValue.Scope,
                                FallbackKey
                            );
                            m_ActiveBindingPairs.pop_back();
                        }
                    }
                    (void)m_Environment->Union(LeftKey, RightKey);
                }
                return;
            }

            if (Left.GetKind() == TypeDesc::Kind::Generic)
            {
                const GenericConstraintKey Key =
                    MakeKey(LeftScope, Left.GetGenericParameter());
                if (!m_Environment->Contains(Key))
                {
                    return;
                }
                const std::size_t Root = m_Environment->FindRoot(Key);
                const GenericConstraintKey* RootKey = m_Environment->GetKey(Root);
                const auto& Existing = m_Environment->GetBinding(Root);
                if (Existing.has_value())
                {
                    if (RootKey == nullptr || IsActiveKey(Root, m_ActiveKeys))
                    {
                        return;
                    }
                    const Binding ExistingValue = *Existing;
                    m_ActiveKeys.push_back(*RootKey);
                    // Expand the binding in its owner scope, not the reference's scope.
                    Relate(
                        ExistingValue.Type,
                        ExistingValue.Scope,
                        Right,
                        RightScope,
                        FallbackKey
                    );
                    m_ActiveKeys.pop_back();
                }
                else
                {
                    Bind(Key, Right, RightScope, FallbackKey);
                }
                return;
            }
            if (Right.GetKind() == TypeDesc::Kind::Generic)
            {
                const GenericConstraintKey Key =
                    MakeKey(RightScope, Right.GetGenericParameter());
                if (!m_Environment->Contains(Key))
                {
                    return;
                }
                const std::size_t Root = m_Environment->FindRoot(Key);
                const GenericConstraintKey* RootKey = m_Environment->GetKey(Root);
                const auto& Existing = m_Environment->GetBinding(Root);
                if (Existing.has_value())
                {
                    if (RootKey == nullptr || IsActiveKey(Root, m_ActiveKeys))
                    {
                        return;
                    }
                    const Binding ExistingValue = *Existing;
                    m_ActiveKeys.push_back(*RootKey);
                    Relate(
                        Left,
                        LeftScope,
                        ExistingValue.Type,
                        ExistingValue.Scope,
                        FallbackKey
                    );
                    m_ActiveKeys.pop_back();
                }
                else
                {
                    Bind(Key, Left, LeftScope, FallbackKey);
                }
                return;
            }

            if (Left.GetKind() != Right.GetKind())
            {
                if (FallbackKey.has_value())
                {
                    AddConflict(*FallbackKey, Left, Right);
                }
                return;
            }

            switch (Left.GetKind())
            {
            case TypeDesc::Kind::List:
                Relate(
                    *Left.GetElementType(), LeftScope,
                    *Right.GetElementType(), RightScope,
                    FallbackKey
                );
                return;
            case TypeDesc::Kind::Dictionary:
                Relate(
                    *Left.GetKeyType(), LeftScope,
                    *Right.GetKeyType(), RightScope,
                    FallbackKey
                );
                Relate(
                    *Left.GetValueType(), LeftScope,
                    *Right.GetValueType(), RightScope,
                    FallbackKey
                );
                return;
            default:
                if (!IsConcreteCompatible(Left, Right) && FallbackKey.has_value())
                {
                    AddConflict(*FallbackKey, Left, Right);
                }
                return;
            }
        }

        void Bind(
            const GenericConstraintKey& Key,
            const TypeDesc& Type,
            const GenericOwnerScope& Scope,
            const std::optional<GenericConstraintKey>& FallbackKey
        )
        {
            if (!m_Environment->Contains(Key))
            {
                return;
            }
            const std::size_t Root = m_Environment->FindRoot(Key);
            const GenericConstraintKey* RootKey = m_Environment->GetKey(Root);
            if (RootKey == nullptr)
            {
                return;
            }
            if (const auto Existing = m_Environment->GetBinding(Root); Existing.has_value())
            {
                if (IsActiveKey(Root, m_ActiveKeys))
                {
                    return;
                }
                const Binding ExistingValue = *Existing;
                m_ActiveKeys.push_back(*RootKey);
                Relate(ExistingValue.Type, ExistingValue.Scope, Type, Scope, FallbackKey);
                m_ActiveKeys.pop_back();
                return;
            }

            if (const auto Generic = FirstGeneric(Type, Scope); Generic.has_value() &&
                m_Environment->FindRoot(*Generic) == Root)
            {
                return;
            }
            (void)m_Environment->SetBinding(Key, Binding{Type, Scope});
        }

        void ValidateResolvedTypes()
        {
            std::vector<GenericConstraintKey> Unresolved;
            for (const GenericConstraintKey& Key : m_Environment->GetKeys())
            {
                const TypeDesc Resolved = Resolve(
                    TypeDesc::Generic(Key.Parameter),
                    Key.Owner,
                    m_ActiveKeys
                );
                if (ContainsGeneric(Resolved))
                {
                    Unresolved.push_back(Key);
                }
            }
            std::sort(Unresolved.begin(), Unresolved.end());
            Unresolved.erase(std::unique(Unresolved.begin(), Unresolved.end()), Unresolved.end());
            m_Unresolved = std::move(Unresolved);
        }

        void AddConflict(
            const GenericConstraintKey& Key,
            const TypeDesc& Left,
            const TypeDesc& Right
        )
        {
            if (!m_Environment->Contains(Key))
            {
                return;
            }
            const GenericConstraintKey* RootKey =
                m_Environment->GetKey(m_Environment->FindRoot(Key));
            if (RootKey == nullptr)
            {
                return;
            }
            Conflict Candidate{*RootKey, Left, Right};
            if (Candidate.Right < Candidate.Left)
            {
                std::swap(Candidate.Left, Candidate.Right);
            }
            if (std::find(m_Conflicts.begin(), m_Conflicts.end(), Candidate) == m_Conflicts.end())
            {
                m_Conflicts.push_back(std::move(Candidate));
            }
        }

        [[nodiscard]] DiagnosticCollection BuildDiagnostics() const
        {
            DiagnosticCollection Diagnostics;
            std::vector<Conflict> Conflicts;
            for (Conflict Candidate : m_Conflicts)
            {
                const GenericConstraintKey* RootKey =
                    m_Environment->GetKey(m_Environment->FindRoot(Candidate.Key));
                if (RootKey == nullptr)
                {
                    continue;
                }
                Candidate.Key = *RootKey;
                if (Candidate.Right < Candidate.Left)
                {
                    std::swap(Candidate.Left, Candidate.Right);
                }
                if (std::find(Conflicts.begin(), Conflicts.end(), Candidate) == Conflicts.end())
                {
                    Conflicts.push_back(std::move(Candidate));
                }
            }
            std::sort(Conflicts.begin(), Conflicts.end());
            for (const Conflict& ConflictValue : Conflicts)
            {
                Diagnostics.push_back(Diagnostic{
                    .Severity = DiagnosticSeverity::Error,
                    .Code = DiagnosticCode::GenericConstraintConflict,
                    .Message = "Generic parameter (" + DescribeKey(ConflictValue.Key) +
                        ") has conflicting concrete constraints (" +
                        DescribeType(ConflictValue.Left) + " vs " +
                        DescribeType(ConflictValue.Right) + ")."
                });
            }

            for (const GenericConstraintKey& Key : m_Unresolved)
            {
                Diagnostics.push_back(Diagnostic{
                    .Severity = DiagnosticSeverity::Error,
                    .Code = DiagnosticCode::UnresolvedGenericType,
                    .Message = "Generic parameter (" + DescribeKey(Key) +
                        ") remains unresolved after constraint solving."
                });
            }
            return Diagnostics;
        }

        const GraphIR& m_Graph;
        const NodeDescriptorRegistry& m_Descriptors;
        Environment* m_Environment = nullptr;
        std::vector<GenericConstraintKey> m_Unresolved;
        std::vector<Conflict> m_Conflicts;
        std::vector<GenericConstraintKey> m_ActiveKeys;
        std::vector<std::pair<std::size_t, std::size_t>> m_ActiveBindingPairs;
    };

    [[nodiscard]] inline DiagnosticCollection ValidateGenericTypes(
        const GraphIR& Graph,
        const NodeDescriptorRegistry& Descriptors
    )
    {
        return Solver(Graph, Descriptors).Run();
    }
}
