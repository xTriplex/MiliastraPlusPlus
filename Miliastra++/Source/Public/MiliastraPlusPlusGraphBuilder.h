#pragma once

#include <algorithm>
#include <cstdint>
#include <expected>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "MiliastraPlusPlusDiagnostics.h"
#include "MiliastraPlusPlusDescriptors.h"
#include "MiliastraPlusPlusGraphIR.h"
#include "MiliastraPlusPlusGraphIRValidation.h"

namespace MiliastraPlusPlus
{
    class GraphBuilder;

    /// Builder-context-bound identity for a graph node; it expires when that context closes.
    class NodeHandle
    {
    public:
        struct Context;

        NodeHandle() = default;

        [[nodiscard]] bool IsValid() const
        {
            return m_Identifier.IsValid() && m_Descriptor.IsValid() && !m_Context.expired();
        }

        [[nodiscard]] NodeInstanceId GetIdentifier() const
        {
            return m_Identifier;
        }

        [[nodiscard]] NodeDescriptorId GetDescriptor() const
        {
            return m_Descriptor;
        }

    private:
        NodeHandle(NodeInstanceId Identifier, NodeDescriptorId Descriptor, const std::shared_ptr<const Context>& ContextToken)
            : m_Identifier(Identifier)
            , m_Descriptor(Descriptor)
            , m_Context(ContextToken)
        {
        }

        friend class GraphBuilder;

        NodeInstanceId m_Identifier;
        NodeDescriptorId m_Descriptor;
        std::weak_ptr<const Context> m_Context;
    };

    struct NodeHandle::Context
    {
    };

    template<typename T>
    struct CppTypeDesc
    {
        static constexpr bool IsSupported = false;

        [[nodiscard]] static std::expected<TypeDesc, DiagnosticCollection> Get()
        {
            return std::unexpected(DiagnosticCollection{
                Diagnostic{
                    .Severity = DiagnosticSeverity::Error,
                    .Code = DiagnosticCode::UnsupportedCppType,
                    .Message = "The requested C++ type is not supported by the graph type bridge."
                }
            });
        }
    };

    template<>
    struct CppTypeDesc<bool>
    {
        static constexpr bool IsSupported = true;

        [[nodiscard]] static std::expected<TypeDesc, DiagnosticCollection> Get()
        {
            return TypeDesc::Boolean();
        }
    };

    template<typename T>
        requires (std::is_integral_v<T> && !std::is_same_v<T, bool>)
    struct CppTypeDesc<T>
    {
        static constexpr bool IsSupported = true;

        [[nodiscard]] static std::expected<TypeDesc, DiagnosticCollection> Get()
        {
            return TypeDesc::Integer();
        }
    };

    template<typename T>
        requires std::is_floating_point_v<T>
    struct CppTypeDesc<T>
    {
        static constexpr bool IsSupported = true;

        [[nodiscard]] static std::expected<TypeDesc, DiagnosticCollection> Get()
        {
            return TypeDesc::Float();
        }
    };

    template<>
    struct CppTypeDesc<std::string>
    {
        static constexpr bool IsSupported = true;

        [[nodiscard]] static std::expected<TypeDesc, DiagnosticCollection> Get()
        {
            return TypeDesc::String();
        }
    };

    template<>
    struct CppTypeDesc<GuidValue>
    {
        static constexpr bool IsSupported = true;

        [[nodiscard]] static std::expected<TypeDesc, DiagnosticCollection> Get()
        {
            return TypeDesc::GUID();
        }
    };

    template<>
    struct CppTypeDesc<Vector3Value>
    {
        static constexpr bool IsSupported = true;

        [[nodiscard]] static std::expected<TypeDesc, DiagnosticCollection> Get()
        {
            return TypeDesc::Vector3();
        }
    };

    template<>
    struct CppTypeDesc<PrefabIdValue>
    {
        static constexpr bool IsSupported = true;

        [[nodiscard]] static std::expected<TypeDesc, DiagnosticCollection> Get()
        {
            return TypeDesc::PrefabId();
        }
    };

    template<>
    struct CppTypeDesc<ConfigIdValue>
    {
        static constexpr bool IsSupported = true;

        [[nodiscard]] static std::expected<TypeDesc, DiagnosticCollection> Get()
        {
            return TypeDesc::ConfigId();
        }
    };

    template<>
    struct CppTypeDesc<FactionValue>
    {
        static constexpr bool IsSupported = true;

        [[nodiscard]] static std::expected<TypeDesc, DiagnosticCollection> Get()
        {
            return TypeDesc::Faction();
        }
    };

    template<>
    struct CppTypeDesc<EntityTypeTag>
    {
        static constexpr bool IsSupported = true;

        [[nodiscard]] static std::expected<TypeDesc, DiagnosticCollection> Get()
        {
            return TypeDesc::Entity();
        }
    };

    template<typename T>
    struct CppTypeDesc<std::vector<T>>
    {
        static constexpr bool IsSupported = CppTypeDesc<T>::IsSupported;

        [[nodiscard]] static std::expected<TypeDesc, DiagnosticCollection> Get()
        {
            auto ElementResult = CppTypeDesc<T>::Get();
            if (!ElementResult.has_value())
            {
                return std::unexpected(std::move(ElementResult.error()));
            }
            return TypeDesc::List(std::move(*ElementResult));
        }
    };

    template<typename T>
    [[nodiscard]] std::expected<TypeDesc, DiagnosticCollection> GetCppTypeDesc()
    {
        return CppTypeDesc<std::remove_cvref_t<T>>::Get();
    }

    template<typename T>
    [[nodiscard]] std::expected<LiteralValue, DiagnosticCollection> MakeLiteralValue(T Value)
    {
        using ValueType = std::remove_cvref_t<T>;
        if constexpr (std::is_same_v<ValueType, bool>)
        {
            return LiteralValue(LiteralValue::Data{Value});
        }
        else if constexpr (std::is_integral_v<ValueType>)
        {
            if constexpr (std::is_unsigned_v<ValueType>)
            {
                if (Value > static_cast<ValueType>(std::numeric_limits<std::int64_t>::max()))
                {
                    return std::unexpected(DiagnosticCollection{
                        Diagnostic{
                            .Severity = DiagnosticSeverity::Error,
                            .Code = DiagnosticCode::IncompatibleGraphIRTypes,
                            .Message = "The integer literal cannot be represented by graph integer storage."
                        }
                    });
                }
            }
            return LiteralValue(LiteralValue::Data{static_cast<std::int64_t>(Value)});
        }
        else if constexpr (std::is_floating_point_v<ValueType>)
        {
            return LiteralValue(LiteralValue::Data{static_cast<double>(Value)});
        }
        else if constexpr (std::is_same_v<ValueType, std::string> ||
            std::is_same_v<ValueType, GuidValue> ||
            std::is_same_v<ValueType, Vector3Value> ||
            std::is_same_v<ValueType, PrefabIdValue> ||
            std::is_same_v<ValueType, ConfigIdValue> ||
            std::is_same_v<ValueType, FactionValue>)
        {
            return LiteralValue(LiteralValue::Data{std::move(Value)});
        }
        else
        {
            return std::unexpected(DiagnosticCollection{
                Diagnostic{
                    .Severity = DiagnosticSeverity::Error,
                    .Code = DiagnosticCode::UnsupportedCppType,
                    .Message = "The requested C++ type has no supported graph literal representation."
                }
            });
        }
    }

    /// Data-output identity. Any explicit type intent is stored on the input binding.
    template<typename T>
    class Output
    {
    public:
        Output() = default;

        [[nodiscard]] bool IsValid() const
        {
            return m_SourceNode.IsValid() && m_SourceOutputPin.IsValid() && !m_Context.expired();
        }

        [[nodiscard]] NodeInstanceId GetIdentifier() const
        {
            return m_SourceNode;
        }

        [[nodiscard]] PinIndex GetPin() const
        {
            return m_SourceOutputPin;
        }

    private:
        Output(NodeInstanceId SourceNode, PinIndex SourceOutputPin, const std::shared_ptr<const NodeHandle::Context>& ContextToken)
            : m_SourceNode(SourceNode)
            , m_SourceOutputPin(SourceOutputPin)
            , m_Context(ContextToken)
        {
        }

        friend class GraphBuilder;

        NodeInstanceId m_SourceNode;
        PinIndex m_SourceOutputPin;
        std::weak_ptr<const NodeHandle::Context> m_Context;
    };

    template<typename T>
    class ValueOrExpr;

    /// Typed graph-variable reference; this API exposes no variable assignment operation.
    template<typename T>
    class Variable
    {
    public:
        Variable() = default;

        [[nodiscard]] bool IsValid() const
        {
            return m_Identifier.IsValid() && !m_Context.expired();
        }

        [[nodiscard]] GraphVariableId GetIdentifier() const
        {
            return m_Identifier;
        }

        [[nodiscard]] ValueOrExpr<T> AsInput() const;

    private:
        Variable(GraphVariableId Identifier, const std::shared_ptr<const NodeHandle::Context>& ContextToken)
            : m_Identifier(Identifier)
            , m_Context(ContextToken)
        {
        }

        friend class GraphBuilder;

        GraphVariableId m_Identifier;
        std::weak_ptr<const NodeHandle::Context> m_Context;
    };

    /// A data input source: literal, output reference, or graph-variable reference.
    template<typename T>
    class ValueOrExpr
    {
    public:
        using Variant = std::variant<LiteralValue, Output<T>, Variable<T>>;

        explicit ValueOrExpr(LiteralValue Literal)
            : m_Value(std::move(Literal))
        {
        }

        explicit ValueOrExpr(Output<T> OutputValue)
            : m_Value(std::move(OutputValue))
        {
        }

        explicit ValueOrExpr(Variable<T> VariableValue)
            : m_Value(std::move(VariableValue))
        {
        }

        [[nodiscard]] const Variant& GetValue() const
        {
            return m_Value;
        }

    private:
        Variant m_Value;
    };

    template<typename T>
    [[nodiscard]] ValueOrExpr<T> Variable<T>::AsInput() const
    {
        return ValueOrExpr<T>(*this);
    }

    /// Explicit live Flow predecessor tied to one builder, entry, and execution region.
    class ExecutionHandle final
    {
    public:
        ExecutionHandle() = default;

        [[nodiscard]] bool IsValid() const
        {
            return m_Node.IsValid() && m_OutputPin.IsValid() && m_Entry.IsValid() &&
                m_Region.IsValid() && !m_Context.expired();
        }

        [[nodiscard]] NodeInstanceId GetSourceNode() const
        {
            return m_Node;
        }

        [[nodiscard]] PinIndex GetSourceOutputPin() const
        {
            return m_OutputPin;
        }

        [[nodiscard]] ExecutionEntryId GetEntry() const
        {
            return m_Entry;
        }

        [[nodiscard]] ExecutionRegionId GetRegion() const
        {
            return m_Region;
        }

    private:
        ExecutionHandle(
            NodeInstanceId Node,
            PinIndex OutputPin,
            ExecutionEntryId Entry,
            ExecutionRegionId Region,
            const std::shared_ptr<const NodeHandle::Context>& ContextToken
        )
            : m_Node(Node)
            , m_OutputPin(OutputPin)
            , m_Entry(Entry)
            , m_Region(Region)
            , m_Context(ContextToken)
        {
        }

        friend class GraphBuilder;

        NodeInstanceId m_Node;
        PinIndex m_OutputPin;
        ExecutionEntryId m_Entry;
        ExecutionRegionId m_Region;
        std::weak_ptr<const NodeHandle::Context> m_Context;
    };

    /// Capability for constructing and closing one structured execution entry.
    class EntryScope final
    {
    public:
        EntryScope() = delete;
        EntryScope(const EntryScope&) = delete;
        EntryScope& operator=(const EntryScope&) = delete;
        EntryScope& operator=(EntryScope&&) = delete;

        EntryScope(EntryScope&& Other) noexcept
            : m_Entry(Other.m_Entry)
            , m_Region(Other.m_Region)
            , m_Root(Other.m_Root)
            , m_OutputPin(Other.m_OutputPin)
            , m_Serial(std::exchange(Other.m_Serial, 0U))
            , m_Context(std::move(Other.m_Context))
        {
            Other.m_Context.reset();
        }

        [[nodiscard]] bool IsValid() const
        {
            return m_Entry.IsValid() && m_Region.IsValid() && m_Root.IsValid() &&
                m_OutputPin.IsValid() && m_Serial != 0U && !m_Context.expired();
        }

    private:
        EntryScope(
            ExecutionEntryId Entry,
            ExecutionRegionId Region,
            NodeInstanceId Root,
            PinIndex OutputPin,
            std::uint64_t Serial,
            const std::shared_ptr<const NodeHandle::Context>& ContextToken
        )
            : m_Entry(Entry)
            , m_Region(Region)
            , m_Root(Root)
            , m_OutputPin(OutputPin)
            , m_Serial(Serial)
            , m_Context(ContextToken)
        {
        }

        friend class GraphBuilder;

        ExecutionEntryId m_Entry;
        ExecutionRegionId m_Region;
        NodeInstanceId m_Root;
        PinIndex m_OutputPin;
        std::uint64_t m_Serial = 0U;
        std::weak_ptr<const NodeHandle::Context> m_Context;
    };

    /// Capability for opening each semantic arm of a branch exactly once.
    class BranchScope final
    {
    public:
        BranchScope() = delete;
        BranchScope(const BranchScope&) = delete;
        BranchScope& operator=(const BranchScope&) = delete;
        BranchScope& operator=(BranchScope&&) = delete;

        BranchScope(BranchScope&& Other) noexcept
            : m_Node(Other.m_Node)
            , m_Entry(Other.m_Entry)
            , m_ParentRegion(Other.m_ParentRegion)
            , m_TrueRegion(Other.m_TrueRegion)
            , m_FalseRegion(Other.m_FalseRegion)
            , m_ParentSerial(Other.m_ParentSerial)
            , m_Serial(std::exchange(Other.m_Serial, 0U))
            , m_Context(std::move(Other.m_Context))
        {
            Other.m_Context.reset();
        }

        [[nodiscard]] bool IsValid() const
        {
            return m_Node.IsValid() && m_Entry.IsValid() && m_ParentRegion.IsValid() &&
                m_TrueRegion.IsValid() && m_FalseRegion.IsValid() && m_Serial != 0U &&
                !m_Context.expired();
        }

    private:
        BranchScope(
            NodeInstanceId Node,
            ExecutionEntryId Entry,
            ExecutionRegionId ParentRegion,
            ExecutionRegionId TrueRegion,
            ExecutionRegionId FalseRegion,
            std::uint64_t ParentSerial,
            std::uint64_t Serial,
            const std::shared_ptr<const NodeHandle::Context>& ContextToken
        )
            : m_Node(Node)
            , m_Entry(Entry)
            , m_ParentRegion(ParentRegion)
            , m_TrueRegion(TrueRegion)
            , m_FalseRegion(FalseRegion)
            , m_ParentSerial(ParentSerial)
            , m_Serial(Serial)
            , m_Context(ContextToken)
        {
        }

        friend class GraphBuilder;

        NodeInstanceId m_Node;
        ExecutionEntryId m_Entry;
        ExecutionRegionId m_ParentRegion;
        ExecutionRegionId m_TrueRegion;
        ExecutionRegionId m_FalseRegion;
        std::uint64_t m_ParentSerial = 0U;
        std::uint64_t m_Serial = 0U;
        std::weak_ptr<const NodeHandle::Context> m_Context;
    };

    enum class BranchArm
    {
        True,
        False
    };

    /// Marks a closed arm as having no live tail, for example after Return or a loop transfer.
    struct NoContinuation final
    {
    };

    /// Capability for building one branch arm in its own execution region.
    class BranchArmScope final
    {
    public:
        BranchArmScope() = delete;
        BranchArmScope(const BranchArmScope&) = delete;
        BranchArmScope& operator=(const BranchArmScope&) = delete;
        BranchArmScope& operator=(BranchArmScope&&) = delete;

        BranchArmScope(BranchArmScope&& Other) noexcept
            : m_Branch(Other.m_Branch)
            , m_Entry(Other.m_Entry)
            , m_ParentRegion(Other.m_ParentRegion)
            , m_Region(Other.m_Region)
            , m_OutputPin(Other.m_OutputPin)
            , m_Arm(Other.m_Arm)
            , m_BranchSerial(Other.m_BranchSerial)
            , m_Serial(std::exchange(Other.m_Serial, 0U))
            , m_Context(std::move(Other.m_Context))
        {
            Other.m_Context.reset();
        }

        [[nodiscard]] bool IsValid() const
        {
            return m_Branch.IsValid() && m_Entry.IsValid() && m_ParentRegion.IsValid() &&
                m_Region.IsValid() && m_OutputPin.IsValid() && m_Serial != 0U &&
                !m_Context.expired();
        }

    private:
        BranchArmScope(
            NodeInstanceId Branch,
            ExecutionEntryId Entry,
            ExecutionRegionId ParentRegion,
            ExecutionRegionId Region,
            PinIndex OutputPin,
            BranchArm Arm,
            std::uint64_t BranchSerial,
            std::uint64_t Serial,
            const std::shared_ptr<const NodeHandle::Context>& ContextToken
        )
            : m_Branch(Branch)
            , m_Entry(Entry)
            , m_ParentRegion(ParentRegion)
            , m_Region(Region)
            , m_OutputPin(OutputPin)
            , m_Arm(Arm)
            , m_BranchSerial(BranchSerial)
            , m_Serial(Serial)
            , m_Context(ContextToken)
        {
        }

        friend class GraphBuilder;

        NodeInstanceId m_Branch;
        ExecutionEntryId m_Entry;
        ExecutionRegionId m_ParentRegion;
        ExecutionRegionId m_Region;
        PinIndex m_OutputPin;
        BranchArm m_Arm = BranchArm::True;
        std::uint64_t m_BranchSerial = 0U;
        std::uint64_t m_Serial = 0U;
        std::weak_ptr<const NodeHandle::Context> m_Context;
    };

    /// Closed arm result, carrying a tail only when that arm still has a continuation.
    class BranchArmOutcome final
    {
    public:
        BranchArmOutcome() = delete;
        BranchArmOutcome(const BranchArmOutcome&) = delete;
        BranchArmOutcome& operator=(const BranchArmOutcome&) = delete;
        BranchArmOutcome& operator=(BranchArmOutcome&&) = delete;

        BranchArmOutcome(BranchArmOutcome&& Other) noexcept
            : m_Branch(Other.m_Branch)
            , m_Entry(Other.m_Entry)
            , m_ParentRegion(Other.m_ParentRegion)
            , m_Region(Other.m_Region)
            , m_Arm(Other.m_Arm)
            , m_LiveTail(std::move(Other.m_LiveTail))
            , m_BranchSerial(Other.m_BranchSerial)
            , m_Serial(std::exchange(Other.m_Serial, 0U))
            , m_Context(std::move(Other.m_Context))
        {
            Other.m_Context.reset();
        }

        [[nodiscard]] bool IsValid() const
        {
            return m_Branch.IsValid() && m_Entry.IsValid() && m_ParentRegion.IsValid() &&
                m_Region.IsValid() && m_Serial != 0U && !m_Context.expired();
        }

    private:
        BranchArmOutcome(
            NodeInstanceId Branch,
            ExecutionEntryId Entry,
            ExecutionRegionId ParentRegion,
            ExecutionRegionId Region,
            BranchArm Arm,
            std::optional<ExecutionHandle> LiveTail,
            std::uint64_t BranchSerial,
            std::uint64_t Serial,
            const std::shared_ptr<const NodeHandle::Context>& ContextToken
        )
            : m_Branch(Branch)
            , m_Entry(Entry)
            , m_ParentRegion(ParentRegion)
            , m_Region(Region)
            , m_Arm(Arm)
            , m_LiveTail(std::move(LiveTail))
            , m_BranchSerial(BranchSerial)
            , m_Serial(Serial)
            , m_Context(ContextToken)
        {
        }

        friend class GraphBuilder;

        NodeInstanceId m_Branch;
        ExecutionEntryId m_Entry;
        ExecutionRegionId m_ParentRegion;
        ExecutionRegionId m_Region;
        BranchArm m_Arm = BranchArm::True;
        std::optional<ExecutionHandle> m_LiveTail;
        std::uint64_t m_BranchSerial = 0U;
        std::uint64_t m_Serial = 0U;
        std::weak_ptr<const NodeHandle::Context> m_Context;
    };

    /// Branch result with zero, one, or two live tails; merges remain explicit.
    class BranchOutcome final
    {
    public:
        BranchOutcome() = delete;
        BranchOutcome(const BranchOutcome&) = delete;
        BranchOutcome& operator=(const BranchOutcome&) = delete;
        BranchOutcome& operator=(BranchOutcome&&) = delete;

        BranchOutcome(BranchOutcome&& Other) noexcept
            : m_Branch(Other.m_Branch)
            , m_Entry(Other.m_Entry)
            , m_ParentRegion(Other.m_ParentRegion)
            , m_TrueRegion(Other.m_TrueRegion)
            , m_FalseRegion(Other.m_FalseRegion)
            , m_TrueTail(std::move(Other.m_TrueTail))
            , m_FalseTail(std::move(Other.m_FalseTail))
            , m_BranchSerial(Other.m_BranchSerial)
            , m_Serial(std::exchange(Other.m_Serial, 0U))
            , m_Context(std::move(Other.m_Context))
        {
            Other.m_Context.reset();
        }

        [[nodiscard]] bool IsValid() const
        {
            return m_Branch.IsValid() && m_Entry.IsValid() && m_ParentRegion.IsValid() &&
                m_TrueRegion.IsValid() && m_FalseRegion.IsValid() && m_Serial != 0U &&
                !m_Context.expired();
        }

        [[nodiscard]] std::size_t GetLiveArmCount() const
        {
            return static_cast<std::size_t>(m_TrueTail.has_value()) +
                static_cast<std::size_t>(m_FalseTail.has_value());
        }

    private:
        BranchOutcome(
            NodeInstanceId Branch,
            ExecutionEntryId Entry,
            ExecutionRegionId ParentRegion,
            ExecutionRegionId TrueRegion,
            ExecutionRegionId FalseRegion,
            std::optional<ExecutionHandle> TrueTail,
            std::optional<ExecutionHandle> FalseTail,
            std::uint64_t BranchSerial,
            std::uint64_t Serial,
            const std::shared_ptr<const NodeHandle::Context>& ContextToken
        )
            : m_Branch(Branch)
            , m_Entry(Entry)
            , m_ParentRegion(ParentRegion)
            , m_TrueRegion(TrueRegion)
            , m_FalseRegion(FalseRegion)
            , m_TrueTail(std::move(TrueTail))
            , m_FalseTail(std::move(FalseTail))
            , m_BranchSerial(BranchSerial)
            , m_Serial(Serial)
            , m_Context(ContextToken)
        {
        }

        friend class GraphBuilder;

        NodeInstanceId m_Branch;
        ExecutionEntryId m_Entry;
        ExecutionRegionId m_ParentRegion;
        ExecutionRegionId m_TrueRegion;
        ExecutionRegionId m_FalseRegion;
        std::optional<ExecutionHandle> m_TrueTail;
        std::optional<ExecutionHandle> m_FalseTail;
        std::uint64_t m_BranchSerial = 0U;
        std::uint64_t m_Serial = 0U;
        std::weak_ptr<const NodeHandle::Context> m_Context;
    };

    /// Capability for building one LoopBody and targeting that nearest active loop.
    class LoopScope final
    {
    public:
        LoopScope() = delete;
        LoopScope(const LoopScope&) = delete;
        LoopScope& operator=(const LoopScope&) = delete;
        LoopScope& operator=(LoopScope&&) = delete;

        LoopScope(LoopScope&& Other) noexcept
            : m_Node(Other.m_Node)
            , m_Entry(Other.m_Entry)
            , m_ParentRegion(Other.m_ParentRegion)
            , m_BodyRegion(Other.m_BodyRegion)
            , m_ParentSerial(Other.m_ParentSerial)
            , m_Serial(std::exchange(Other.m_Serial, 0U))
            , m_Context(std::move(Other.m_Context))
        {
            Other.m_Context.reset();
        }

        [[nodiscard]] bool IsValid() const
        {
            return m_Node.IsValid() && m_Entry.IsValid() && m_ParentRegion.IsValid() &&
                m_BodyRegion.IsValid() && m_Serial != 0U && !m_Context.expired();
        }

    private:
        LoopScope(
            NodeInstanceId Node,
            ExecutionEntryId Entry,
            ExecutionRegionId ParentRegion,
            ExecutionRegionId BodyRegion,
            std::uint64_t ParentSerial,
            std::uint64_t Serial,
            const std::shared_ptr<const NodeHandle::Context>& ContextToken
        )
            : m_Node(Node)
            , m_Entry(Entry)
            , m_ParentRegion(ParentRegion)
            , m_BodyRegion(BodyRegion)
            , m_ParentSerial(ParentSerial)
            , m_Serial(Serial)
            , m_Context(ContextToken)
        {
        }

        friend class GraphBuilder;

        NodeInstanceId m_Node;
        ExecutionEntryId m_Entry;
        ExecutionRegionId m_ParentRegion;
        ExecutionRegionId m_BodyRegion;
        std::uint64_t m_ParentSerial = 0U;
        std::uint64_t m_Serial = 0U;
        std::weak_ptr<const NodeHandle::Context> m_Context;
    };

    struct EntryStart final
    {
        EntryScope Scope;
        NodeHandle Root;
        ExecutionHandle RootOutput;
    };

    struct ExecutionNodeResult final
    {
        NodeHandle Node;
        ExecutionHandle Output;
    };

    struct BranchStart final
    {
        BranchScope Scope;
        NodeHandle Node;
    };

    struct BranchArmStart final
    {
        BranchArmScope Scope;
        ExecutionHandle ArmOutput;
    };

    struct JoinResult final
    {
        NodeHandle Node;
        ExecutionHandle Output;
    };

    struct LoopStart final
    {
        LoopScope Scope;
        NodeHandle Node;
        ExecutionHandle BodyOutput;
    };

    struct LoopResult final
    {
        // Conditional loops retain their condition-false exit. Unconditional loops
        // expose an exit only after a reachable Break.
        std::optional<ExecutionHandle> ExitOutput;
    };

    /// Builds canonical GraphIR using explicit predecessors and context-bound scope capabilities.
    class GraphBuilder
    {
    public:
        explicit GraphBuilder(const NodeDescriptorRegistry& Descriptors)
            : m_Descriptors(Descriptors)
            , m_Context(std::make_shared<const NodeHandle::Context>())
        {
        }

        GraphBuilder(const GraphBuilder&) = delete;
        GraphBuilder& operator=(const GraphBuilder&) = delete;

        // The context token moves with the graph so active handles and scopes remain usable.
        GraphBuilder(GraphBuilder&& Other) noexcept
            : m_Descriptors(Other.m_Descriptors)
            , m_Graph(std::move(Other.m_Graph))
            , m_NextNodeIdentifier(Other.m_NextNodeIdentifier)
            , m_NextGraphVariableIdentifier(Other.m_NextGraphVariableIdentifier)
            , m_NextExecutionEntryIdentifier(Other.m_NextExecutionEntryIdentifier)
            , m_NextExecutionRegionIdentifier(Other.m_NextExecutionRegionIdentifier)
            , m_NextExecutionScopeSerial(Other.m_NextExecutionScopeSerial)
            , m_NextBranchOutcomeSerial(Other.m_NextBranchOutcomeSerial)
            , m_ExecutionScopes(std::move(Other.m_ExecutionScopes))
            , m_BranchStates(std::move(Other.m_BranchStates))
            , m_OpenBranchOutcomes(std::move(Other.m_OpenBranchOutcomes))
            , m_Context(std::move(Other.m_Context))
            , m_IsClosed(Other.m_IsClosed)
        {
            Other.m_IsClosed = true;
            Other.m_Context.reset();
        }

        GraphBuilder& operator=(GraphBuilder&&) = delete;

        /// Starts a structured entry with a real Entry root and its root Flow output.
        [[nodiscard]] std::expected<EntryStart, DiagnosticCollection> BeginEntry(NodeDescriptorId EntryDescriptorIdentifier)
        {
            if (m_IsClosed)
            {
                return std::unexpected(DiagnosticCollection{
                    MakeLifecycleDiagnostic("A closed graph builder cannot begin an execution entry.")
                });
            }
            if (!m_ExecutionScopes.empty() || !m_OpenBranchOutcomes.empty())
            {
                return std::unexpected(DiagnosticCollection{
                    MakeExecutionScopeDiagnostic("Only one execution entry scope may be active at a time.")
                });
            }
            const NodeDescriptor* Descriptor = FindValidDescriptor(EntryDescriptorIdentifier);
            if (Descriptor == nullptr)
            {
                return std::unexpected(DescriptorFailure(EntryDescriptorIdentifier,
                    "The requested Entry descriptor is missing or invalid."));
            }
            const EntryControlSchema* Schema = GetControlSchema<EntryControlSchema>(*Descriptor);
            if (Schema == nullptr)
            {
                return std::unexpected(DiagnosticCollection{
                    MakeExecutionRoleDiagnostic("BeginEntry requires a trusted Entry control schema.")
                });
            }
            for (const NodeInstance& Node : m_Graph.GetNodes())
            {
                const NodeDescriptor* ExistingDescriptor = Node.Descriptor.IsValid()
                    ? m_Descriptors.Find(Node.Descriptor) : nullptr;
                if (m_Graph.GetExecutionModel() == ExecutionModel::Unstructured &&
                    ExistingDescriptor != nullptr && HasFlowPins(*ExistingDescriptor))
                {
                    return std::unexpected(DiagnosticCollection{
                        MakeExecutionOwnershipDiagnostic(
                            "Existing Flow nodes cannot be adopted when structured construction begins.")
                    });
                }
            }
            if (m_NextNodeIdentifier == 0U || m_NextExecutionEntryIdentifier == 0U ||
                m_NextExecutionRegionIdentifier == 0U || m_NextExecutionScopeSerial == 0U)
            {
                return std::unexpected(DiagnosticCollection{
                    MakeLifecycleDiagnostic("The graph builder exhausted an identifier range.")
                });
            }

            const NodeInstanceId NodeIdentifier(m_NextNodeIdentifier);
            const ExecutionEntryId EntryIdentifier(m_NextExecutionEntryIdentifier);
            const ExecutionRegionId RegionIdentifier(m_NextExecutionRegionIdentifier);
            const std::uint64_t ScopeSerial = m_NextExecutionScopeSerial;

            m_Graph.SetExecutionModel(ExecutionModel::Structured);
            m_Graph.AddNode(NodeInstance{NodeIdentifier, EntryDescriptorIdentifier, RegionIdentifier});
            m_Graph.AddExecutionEntry(ExecutionEntry{EntryIdentifier, NodeIdentifier});
            m_Graph.AddExecutionRegion(ExecutionRegion{
                RegionIdentifier,
                EntryIdentifier,
                ExecutionRegionKind::Entry,
                std::nullopt,
                std::nullopt,
                std::nullopt
            });
            m_ExecutionScopes.push_back(ExecutionScopeFrame{
                ExecutionScopeKind::Entry,
                ScopeSerial,
                0U,
                EntryIdentifier,
                RegionIdentifier,
                NodeIdentifier
            });
            AdvanceIdentifier(m_NextNodeIdentifier);
            AdvanceIdentifier(m_NextExecutionEntryIdentifier);
            AdvanceIdentifier(m_NextExecutionRegionIdentifier);
            AdvanceIdentifier(m_NextExecutionScopeSerial);

            EntryScope Scope(EntryIdentifier, RegionIdentifier, NodeIdentifier,
                Schema->ExecutionOutput, ScopeSerial, m_Context);
            NodeHandle Root(NodeIdentifier, EntryDescriptorIdentifier, m_Context);
            ExecutionHandle RootOutput(NodeIdentifier, Schema->ExecutionOutput,
                EntryIdentifier, RegionIdentifier, m_Context);
            return EntryStart{std::move(Scope), std::move(Root), std::move(RootOutput)};
        }

        /// Closes a resolved entry; the root needs an outgoing action, but paths
        /// need not Return.
        [[nodiscard]] std::expected<void, DiagnosticCollection> EndEntry(EntryScope&& Scope)
        {
            if (!IsBuilderOpen())
            {
                return std::unexpected(DiagnosticCollection{
                    MakeLifecycleDiagnostic("A closed graph builder cannot end an execution entry.")
                });
            }
            if (!Scope.IsValid())
            {
                return std::unexpected(DiagnosticCollection{
                    MakeExecutionScopeDiagnostic("The EntryScope is invalid or already closed.")
                });
            }
            if (!HasSameContext(Scope.m_Context))
            {
                return std::unexpected(DiagnosticCollection{
                    MakeForeignContextDiagnostic("The EntryScope belongs to another builder.")
                });
            }
            if (!IsTopScope(ExecutionScopeKind::Entry, Scope.m_Serial) ||
                m_ExecutionScopes.back().Entry != Scope.m_Entry ||
                m_ExecutionScopes.back().Region != Scope.m_Region)
            {
                return std::unexpected(DiagnosticCollection{
                    MakeExecutionScopeDiagnostic("The EntryScope is not the active innermost scope.")
                });
            }
            if (HasOpenBranchOutcome(Scope.m_Entry))
            {
                return std::unexpected(DiagnosticCollection{
                    MakeBranchOutcomeDiagnostic("A branch outcome must be joined, continued, or closed before EndEntry.")
                });
            }
            const std::size_t SuccessorCount = CountOutgoingEdges(Scope.m_Root, Scope.m_OutputPin);
            if (SuccessorCount == 0U)
            {
                return std::unexpected(DiagnosticCollection{
                    MakeExecutionReachabilityDiagnostic(
                        "A structured Entry root must connect to an execution node before EndEntry.")
                });
            }

            m_ExecutionScopes.pop_back();
            Scope.m_Serial = 0U;
            Scope.m_Context.reset();
            return {};
        }

        /// Appends a Sequence in the active region and returns its explicit live output.
        [[nodiscard]] std::expected<ExecutionNodeResult, DiagnosticCollection> AppendExecutionNode(
            EntryScope& Scope,
            const ExecutionHandle& Predecessor,
            NodeDescriptorId SequenceDescriptorIdentifier
        )
        {
            return AppendSequence(Scope.m_Entry, Scope.m_Region, Scope.m_Serial,
                Scope.m_Context, Predecessor, SequenceDescriptorIdentifier);
        }

        [[nodiscard]] std::expected<ExecutionNodeResult, DiagnosticCollection> AppendExecutionNode(
            BranchArmScope& Scope,
            const ExecutionHandle& Predecessor,
            NodeDescriptorId SequenceDescriptorIdentifier
        )
        {
            return AppendSequence(Scope.m_Entry, Scope.m_Region, Scope.m_Serial,
                Scope.m_Context, Predecessor, SequenceDescriptorIdentifier);
        }

        [[nodiscard]] std::expected<ExecutionNodeResult, DiagnosticCollection> AppendExecutionNode(
            LoopScope& Scope,
            const ExecutionHandle& Predecessor,
            NodeDescriptorId SequenceDescriptorIdentifier
        )
        {
            return AppendSequence(Scope.m_Entry, Scope.m_BodyRegion, Scope.m_Serial,
                Scope.m_Context, Predecessor, SequenceDescriptorIdentifier);
        }

        /// Appends a valueless Return and consumes Tail. Success produces no continuation handle.
        [[nodiscard]] std::expected<void, DiagnosticCollection> Return(EntryScope& Scope, const ExecutionHandle& Tail, NodeDescriptorId ReturnDescriptorIdentifier)
        {
            return ReturnInScope(ExecutionScopeKind::Entry, Scope.m_Entry, Scope.m_Region,
                Scope.m_Serial, Scope.m_Context, Tail, ReturnDescriptorIdentifier);
        }

        [[nodiscard]] std::expected<void, DiagnosticCollection> Return(BranchArmScope& Scope, const ExecutionHandle& Tail, NodeDescriptorId ReturnDescriptorIdentifier)
        {
            return ReturnInScope(ExecutionScopeKind::BranchArm, Scope.m_Entry, Scope.m_Region,
                Scope.m_Serial, Scope.m_Context, Tail, ReturnDescriptorIdentifier);
        }

        [[nodiscard]] std::expected<void, DiagnosticCollection> Return(LoopScope& Scope, const ExecutionHandle& Tail, NodeDescriptorId ReturnDescriptorIdentifier)
        {
            return ReturnInScope(ExecutionScopeKind::Loop, Scope.m_Entry, Scope.m_BodyRegion,
                Scope.m_Serial, Scope.m_Context, Tail, ReturnDescriptorIdentifier);
        }

        [[nodiscard]] std::expected<BranchStart, DiagnosticCollection> BeginBranch(
            EntryScope& Parent,
            const ExecutionHandle& Predecessor,
            NodeDescriptorId BranchDescriptorIdentifier,
            const ValueOrExpr<bool>& Condition
        )
        {
            return BeginBranchInScope(Parent.m_Entry, Parent.m_Region, Parent.m_Serial,
                Parent.m_Context, Predecessor, BranchDescriptorIdentifier, Condition);
        }

        [[nodiscard]] std::expected<BranchStart, DiagnosticCollection> BeginBranch(
            BranchArmScope& Parent,
            const ExecutionHandle& Predecessor,
            NodeDescriptorId BranchDescriptorIdentifier,
            const ValueOrExpr<bool>& Condition
        )
        {
            return BeginBranchInScope(Parent.m_Entry, Parent.m_Region, Parent.m_Serial,
                Parent.m_Context, Predecessor, BranchDescriptorIdentifier, Condition);
        }

        [[nodiscard]] std::expected<BranchStart, DiagnosticCollection> BeginBranch(
            LoopScope& Parent,
            const ExecutionHandle& Predecessor,
            NodeDescriptorId BranchDescriptorIdentifier,
            const ValueOrExpr<bool>& Condition
        )
        {
            return BeginBranchInScope(Parent.m_Entry, Parent.m_BodyRegion, Parent.m_Serial,
                Parent.m_Context, Predecessor, BranchDescriptorIdentifier, Condition);
        }

        /// Opens one arm; true and false arms may be built in either order.
        [[nodiscard]] std::expected<BranchArmStart, DiagnosticCollection> BeginArm(BranchScope& Branch, BranchArm Arm)
        {
            if (!IsBuilderOpen())
            {
                return std::unexpected(DiagnosticCollection{
                    MakeLifecycleDiagnostic("A closed graph builder cannot begin a branch arm.")
                });
            }
            if (!Branch.IsValid())
            {
                return std::unexpected(DiagnosticCollection{
                    MakeExecutionScopeDiagnostic("The BranchScope is invalid or already closed.")
                });
            }
            if (!HasSameContext(Branch.m_Context))
            {
                return std::unexpected(DiagnosticCollection{
                    MakeForeignContextDiagnostic("The BranchScope belongs to another builder.")
                });
            }
            if (!IsTopScope(ExecutionScopeKind::Branch, Branch.m_Serial))
            {
                return std::unexpected(DiagnosticCollection{
                    MakeExecutionScopeDiagnostic("The BranchScope is not the active innermost scope.")
                });
            }
            BranchConstructionState* State = FindBranchState(Branch.m_Serial);
            if (State == nullptr || !IsValidBranchArm(Arm))
            {
                return std::unexpected(DiagnosticCollection{
                    MakeBranchArmDiagnostic("The BranchScope or semantic arm identity is invalid.")
                });
            }
            bool& WasOpened = Arm == BranchArm::True ? State->TrueOpened : State->FalseOpened;
            bool& WasResolved = Arm == BranchArm::True ? State->TrueResolved : State->FalseResolved;
            if (WasOpened || WasResolved)
            {
                return std::unexpected(DiagnosticCollection{
                    MakeBranchArmDiagnostic("Each semantic branch arm may be opened exactly once.")
                });
            }
            if (m_NextExecutionScopeSerial == 0U)
            {
                return std::unexpected(DiagnosticCollection{
                    MakeLifecycleDiagnostic("The graph builder exhausted its execution scope serial range.")
                });
            }

            const NodeInstance* BranchNode = m_Graph.FindNode(Branch.m_Node);
            const NodeDescriptor* Descriptor = BranchNode == nullptr
                ? nullptr : m_Descriptors.Find(BranchNode->Descriptor);
            const BranchControlSchema* Schema = Descriptor == nullptr
                ? nullptr : GetControlSchema<BranchControlSchema>(*Descriptor);
            if (Schema == nullptr)
            {
                return std::unexpected(DiagnosticCollection{
                    MakeExecutionRoleDiagnostic("The Branch descriptor no longer has a valid Branch control schema.")
                });
            }
            const ExecutionRegionId Region = Arm == BranchArm::True
                ? Branch.m_TrueRegion : Branch.m_FalseRegion;
            const PinIndex OutputPin = Arm == BranchArm::True
                ? Schema->TrueOutput : Schema->FalseOutput;
            const std::uint64_t ScopeSerial = m_NextExecutionScopeSerial;
            WasOpened = true;
            m_ExecutionScopes.push_back(ExecutionScopeFrame{
                ExecutionScopeKind::BranchArm,
                ScopeSerial,
                Branch.m_Serial,
                Branch.m_Entry,
                Region,
                Branch.m_Node
            });
            AdvanceIdentifier(m_NextExecutionScopeSerial);

            BranchArmScope Scope(Branch.m_Node, Branch.m_Entry, Branch.m_ParentRegion,
                Region, OutputPin, Arm, Branch.m_Serial, ScopeSerial, m_Context);
            ExecutionHandle ArmOutput(Branch.m_Node, OutputPin, Branch.m_Entry,
                Region, m_Context);
            return BranchArmStart{std::move(Scope), std::move(ArmOutput)};
        }

        /// Closes an arm with its live tail. Use NoContinuation when the path has already terminated.
        [[nodiscard]] std::expected<BranchArmOutcome, DiagnosticCollection> EndArm(BranchArmScope&& Arm, const ExecutionHandle& LiveTail)
        {
            return EndBranchArm(std::move(Arm), &LiveTail);
        }

        [[nodiscard]] std::expected<BranchArmOutcome, DiagnosticCollection> EndArm(BranchArmScope&& Arm, NoContinuation)
        {
            return EndBranchArm(std::move(Arm), nullptr);
        }

        /// Combines the two closed arms without implicitly joining their live tails.
        [[nodiscard]] std::expected<BranchOutcome, DiagnosticCollection> EndBranch(BranchScope&& Branch, BranchArmOutcome&& TrueArm, BranchArmOutcome&& FalseArm)
        {
            return EndBranchConstruction(std::move(Branch), std::move(TrueArm), std::move(FalseArm));
        }

        /// Materializes the required merge for a branch with two live arms.
        [[nodiscard]] std::expected<JoinResult, DiagnosticCollection> Join(EntryScope& Parent, BranchOutcome&& TwoLiveArms, NodeDescriptorId JoinDescriptorIdentifier)
        {
            return JoinBranchOutcome(Parent.m_Entry, Parent.m_Region, Parent.m_Serial,
                Parent.m_Context, std::move(TwoLiveArms), JoinDescriptorIdentifier);
        }

        [[nodiscard]] std::expected<JoinResult, DiagnosticCollection> Join(BranchArmScope& Parent, BranchOutcome&& TwoLiveArms, NodeDescriptorId JoinDescriptorIdentifier)
        {
            return JoinBranchOutcome(Parent.m_Entry, Parent.m_Region, Parent.m_Serial,
                Parent.m_Context, std::move(TwoLiveArms), JoinDescriptorIdentifier);
        }

        /// Adds a parent-region Sequence for the sole live arm of a branch.
        [[nodiscard]] std::expected<ExecutionNodeResult, DiagnosticCollection> ContinueWith(
            EntryScope& Parent,
            BranchOutcome&& OneLiveArm,
            NodeDescriptorId SequenceDescriptorIdentifier
        )
        {
            return ContinueBranchOutcome(Parent.m_Entry, Parent.m_Region, Parent.m_Serial,
                Parent.m_Context, std::move(OneLiveArm), SequenceDescriptorIdentifier);
        }

        [[nodiscard]] std::expected<ExecutionNodeResult, DiagnosticCollection> ContinueWith(
            BranchArmScope& Parent,
            BranchOutcome&& OneLiveArm,
            NodeDescriptorId SequenceDescriptorIdentifier
        )
        {
            return ContinueBranchOutcome(Parent.m_Entry, Parent.m_Region, Parent.m_Serial,
                Parent.m_Context, std::move(OneLiveArm), SequenceDescriptorIdentifier);
        }

        [[nodiscard]] std::expected<JoinResult, DiagnosticCollection> Join(LoopScope& Parent, BranchOutcome&& TwoLiveArms, NodeDescriptorId JoinDescriptorIdentifier)
        {
            return JoinBranchOutcome(Parent.m_Entry, Parent.m_BodyRegion, Parent.m_Serial,
                Parent.m_Context, std::move(TwoLiveArms), JoinDescriptorIdentifier);
        }

        [[nodiscard]] std::expected<ExecutionNodeResult, DiagnosticCollection> ContinueWith(
            LoopScope& Parent,
            BranchOutcome&& OneLiveArm,
            NodeDescriptorId SequenceDescriptorIdentifier
        )
        {
            return ContinueBranchOutcome(Parent.m_Entry, Parent.m_BodyRegion, Parent.m_Serial,
                Parent.m_Context, std::move(OneLiveArm), SequenceDescriptorIdentifier);
        }

        /// Starts a loop whose body is built in a child LoopBody region.
        [[nodiscard]] std::expected<LoopStart, DiagnosticCollection> BeginLoop(
            EntryScope& Parent,
            const ExecutionHandle& Predecessor,
            NodeDescriptorId LoopDescriptorIdentifier,
            std::optional<ValueOrExpr<bool>> Condition = std::nullopt
        )
        {
            return BeginLoopInScope(Parent.m_Entry, Parent.m_Region, Parent.m_Serial,
                Parent.m_Context, Predecessor, LoopDescriptorIdentifier, std::move(Condition));
        }

        [[nodiscard]] std::expected<LoopStart, DiagnosticCollection> BeginLoop(
            BranchArmScope& Parent,
            const ExecutionHandle& Predecessor,
            NodeDescriptorId LoopDescriptorIdentifier,
            std::optional<ValueOrExpr<bool>> Condition = std::nullopt
        )
        {
            return BeginLoopInScope(Parent.m_Entry, Parent.m_Region, Parent.m_Serial,
                Parent.m_Context, Predecessor, LoopDescriptorIdentifier, std::move(Condition));
        }

        [[nodiscard]] std::expected<LoopStart, DiagnosticCollection> BeginLoop(
            LoopScope& Parent,
            const ExecutionHandle& Predecessor,
            NodeDescriptorId LoopDescriptorIdentifier,
            std::optional<ValueOrExpr<bool>> Condition = std::nullopt
        )
        {
            return BeginLoopInScope(Parent.m_Entry, Parent.m_BodyRegion, Parent.m_Serial,
                Parent.m_Context, Predecessor, LoopDescriptorIdentifier, std::move(Condition));
        }

        /// Consumes Tail into the nearest active loop's BreakInput.
        [[nodiscard]] std::expected<void, DiagnosticCollection> Break(LoopScope& NearestLoop, const ExecutionHandle& Tail)
        {
            return AddLoopTransfer(NearestLoop, Tail, true);
        }

        /// Consumes Tail into the nearest active loop's RepeatInput.
        [[nodiscard]] std::expected<void, DiagnosticCollection> Continue(LoopScope& NearestLoop, const ExecutionHandle& Tail)
        {
            return AddLoopTransfer(NearestLoop, Tail, false);
        }

        /// Closes a resolved body without adding an implicit Repeat edge.
        [[nodiscard]] std::expected<LoopResult, DiagnosticCollection> EndLoop(LoopScope&& Scope)
        {
            if (!IsBuilderOpen())
            {
                return std::unexpected(DiagnosticCollection{
                    MakeLifecycleDiagnostic("A closed graph builder cannot end a loop.")
                });
            }
            if (!Scope.IsValid())
            {
                return std::unexpected(DiagnosticCollection{
                    MakeExecutionScopeDiagnostic("The LoopScope is invalid or already closed.")
                });
            }
            if (!HasSameContext(Scope.m_Context))
            {
                return std::unexpected(DiagnosticCollection{
                    MakeForeignContextDiagnostic("The LoopScope belongs to another builder.")
                });
            }
            if (!IsTopScope(ExecutionScopeKind::Loop, Scope.m_Serial))
            {
                return std::unexpected(DiagnosticCollection{
                    MakeExecutionScopeDiagnostic("The LoopScope is not the active innermost scope.")
                });
            }
            const ExecutionScopeFrame& Frame = m_ExecutionScopes.back();
            if (Frame.Entry != Scope.m_Entry || Frame.Region != Scope.m_BodyRegion ||
                Frame.OwnerNode != Scope.m_Node || Frame.ParentSerial != Scope.m_ParentSerial)
            {
                return std::unexpected(DiagnosticCollection{
                    MakeExecutionScopeDiagnostic("The LoopScope does not match the active LoopBody frame.")
                });
            }
            if (HasOpenBranchOutcomeInRegion(Scope.m_BodyRegion))
            {
                return std::unexpected(DiagnosticCollection{
                    MakeBranchOutcomeDiagnostic("Resolve every live branch outcome before EndLoop.")
                });
            }
            const NodeInstance* LoopNode = m_Graph.FindNode(Scope.m_Node);
            const NodeDescriptor* Descriptor = LoopNode == nullptr
                ? nullptr : m_Descriptors.Find(LoopNode->Descriptor);
            const LoopControlSchema* Schema = Descriptor == nullptr
                ? nullptr : GetControlSchema<LoopControlSchema>(*Descriptor);
            const ExecutionRegion* BodyRegion = m_Graph.FindExecutionRegion(Scope.m_BodyRegion);
            if (LoopNode == nullptr || Schema == nullptr || BodyRegion == nullptr ||
                BodyRegion->Kind != ExecutionRegionKind::LoopBody ||
                BodyRegion->OwnerNode != Scope.m_Node ||
                BodyRegion->Parent != Scope.m_ParentRegion ||
                BodyRegion->Entry != Scope.m_Entry)
            {
                return std::unexpected(DiagnosticCollection{
                    MakeExecutionOwnershipDiagnostic("The active LoopBody ownership is invalid.")
                });
            }
            const ControlEdge* BodyAction = nullptr;
            for (const ControlEdge& Edge : m_Graph.GetControlEdges())
            {
                if (Edge.SourceNode == Scope.m_Node && Edge.SourceOutputPin == Schema->BodyOutput)
                {
                    if (BodyAction != nullptr)
                    {
                        return std::unexpected(DiagnosticCollection{
                            MakeExecutionReachabilityDiagnostic("A Loop Body output must have exactly one legal first body action.")
                        });
                    }
                    BodyAction = &Edge;
                }
            }
            if (BodyAction == nullptr)
            {
                return std::unexpected(DiagnosticCollection{
                    MakeExecutionReachabilityDiagnostic("A Loop Body output must have one legal first body action.")
                });
            }
            const bool IsDirectTransfer = BodyAction->DestinationNode == Scope.m_Node &&
                (BodyAction->DestinationInputPin == Schema->BreakInput ||
                    BodyAction->DestinationInputPin == Schema->RepeatInput);
            if (!IsDirectTransfer)
            {
                const NodeInstance* BodyRoot = m_Graph.FindNode(BodyAction->DestinationNode);
                const NodeDescriptor* BodyRootDescriptor = BodyRoot == nullptr
                    ? nullptr : m_Descriptors.Find(BodyRoot->Descriptor);
                if (BodyRoot == nullptr || BodyRoot->ExecutionRegion != Scope.m_BodyRegion ||
                    BodyRootDescriptor == nullptr ||
                    !BodyRootDescriptor->GetExecutionControlSchema().has_value())
                {
                    return std::unexpected(DiagnosticCollection{
                        MakeExecutionReachabilityDiagnostic(
                            "A Loop Body output must enter its owned LoopBody or directly transfer to its own Break or Repeat input.")
                    });
                }
            }

            const bool HasReachableBreak = HasReachableLoopBreak(
                Scope.m_Node, *Schema, Scope.m_BodyRegion);
            const bool HasExit = Schema->ExitPolicy == LoopExitPolicy::Conditional || HasReachableBreak;
            std::optional<ExecutionHandle> ExitOutput;
            if (HasExit)
            {
                ExecutionHandle Handle(Scope.m_Node, Schema->ExitOutput, Scope.m_Entry,
                    Scope.m_ParentRegion, m_Context);
                ExitOutput = std::move(Handle);
            }

            m_ExecutionScopes.pop_back();
            Scope.m_Serial = 0U;
            Scope.m_Context.reset();
            return LoopResult{std::move(ExitOutput)};
        }

        [[nodiscard]] std::expected<NodeHandle, DiagnosticCollection> AddNode(NodeDescriptorId DescriptorIdentifier)
        {
            if (m_IsClosed)
            {
                return std::unexpected(DiagnosticCollection{
                    MakeLifecycleDiagnostic("A closed graph builder cannot add a node.")
                });
            }

            if (!DescriptorIdentifier.IsValid())
            {
                return std::unexpected(DiagnosticCollection{
                    MakeDescriptorDiagnostic("An invalid node descriptor identifier was supplied.")
                });
            }

            const NodeDescriptor* Descriptor = m_Descriptors.Find(DescriptorIdentifier);
            if (Descriptor == nullptr)
            {
                return std::unexpected(DiagnosticCollection{
                    MakeMissingDescriptorDiagnostic("The requested node descriptor is not registered.")
                });
            }

            if (!Descriptor->IsValid())
            {
                return std::unexpected(DiagnosticCollection{
                    MakeDescriptorDiagnostic("The requested node descriptor is invalid.")
                });
            }
            if (m_Graph.GetExecutionModel() == ExecutionModel::Structured &&
                HasFlowPins(*Descriptor))
            {
                return std::unexpected(DiagnosticCollection{
                    MakeExecutionOwnershipDiagnostic(
                        "Flow-bearing nodes in a Structured graph must use the execution construction API.")
                });
            }

            if (m_NextNodeIdentifier == 0U)
            {
                return std::unexpected(DiagnosticCollection{
                    MakeLifecycleDiagnostic("The graph builder exhausted its node identifier range.")
                });
            }

            const NodeInstanceId Identifier(m_NextNodeIdentifier);
            m_Graph.AddNode(NodeInstance{Identifier, DescriptorIdentifier});

            if (m_NextNodeIdentifier == std::numeric_limits<std::uint64_t>::max())
            {
                m_NextNodeIdentifier = 0U;
            }
            else
            {
                ++m_NextNodeIdentifier;
            }

            return NodeHandle(Identifier, DescriptorIdentifier, m_Context);
        }

        template<typename T>
        [[nodiscard]] std::expected<Output<T>, DiagnosticCollection> GetOutput(const NodeHandle& Node, PinIndex OutputPin) const
        {
            const auto TypeResult = GetCppTypeDesc<T>();
            if (!TypeResult.has_value())
            {
                return std::unexpected(TypeResult.error());
            }
            if (!IsHandleUsable(Node))
            {
                return std::unexpected(DiagnosticCollection{
                    MakeBindingDiagnostic("The output source handle is invalid or belongs to another builder.")
                });
            }

            const NodeInstance* SourceNode = m_Graph.FindNode(Node.GetIdentifier());
            const NodeDescriptor* Descriptor = m_Descriptors.Find(SourceNode->Descriptor);
            if (Descriptor == nullptr)
            {
                return std::unexpected(DiagnosticCollection{
                    MakeMissingDescriptorDiagnostic(
                        "The output source node descriptor is no longer registered.")
                });
            }
            if (!OutputPin.IsValid() || OutputPin.GetValue() >= Descriptor->GetPins().size())
            {
                return std::unexpected(DiagnosticCollection{
                    MakePinDiagnostic("The requested output pin does not exist.")
                });
            }

            const PinSchema& Pin = Descriptor->GetPins()[OutputPin.GetValue()];
            if (Pin.GetDirection() != PinDirection::Output || Pin.GetCategory() != PinCategory::Data)
            {
                return std::unexpected(DiagnosticCollection{
                    MakeBindingDiagnostic("A typed output must reference a data output pin.")
                });
            }
            if (!Pin.GetType().IsCompatibleWith(*TypeResult))
            {
                return std::unexpected(DiagnosticCollection{
                    MakeTypeDiagnostic("The requested C++ output type is incompatible with the descriptor pin type.")
                });
            }

            return Output<T>(Node.GetIdentifier(), OutputPin, m_Context);
        }

        template<typename T>
        [[nodiscard]] std::expected<Variable<T>, DiagnosticCollection> DeclareVariable(std::string Name, std::optional<LiteralValue> DefaultValue = std::nullopt)
        {
            if (m_IsClosed)
            {
                return std::unexpected(DiagnosticCollection{
                    MakeLifecycleDiagnostic("A closed graph builder cannot declare a variable.")
                });
            }

            const auto TypeResult = GetCppTypeDesc<T>();
            if (!TypeResult.has_value())
            {
                return std::unexpected(TypeResult.error());
            }
            if (Name.empty())
            {
                return std::unexpected(DiagnosticCollection{
                    MakeVariableDiagnostic("A graph variable name cannot be empty.")
                });
            }
            for (const GraphVariable& VariableRecord : m_Graph.GetVariables())
            {
                if (VariableRecord.Name == Name)
                {
                    return std::unexpected(DiagnosticCollection{
                        MakeVariableNameDiagnostic("A graph variable with this name already exists.")
                    });
                }
            }
            if (DefaultValue.has_value() && !IsLiteralCompatible(*DefaultValue, *TypeResult))
            {
                return std::unexpected(DiagnosticCollection{
                    MakeTypeDiagnostic("The graph variable default is incompatible with its declared C++ type.")
                });
            }
            if (m_NextGraphVariableIdentifier == 0U)
            {
                return std::unexpected(DiagnosticCollection{
                    MakeLifecycleDiagnostic("The graph builder exhausted its graph variable identifier range.")
                });
            }

            const GraphVariableId Identifier(m_NextGraphVariableIdentifier);
            m_Graph.AddVariable(GraphVariable{
                Identifier,
                std::move(Name),
                *TypeResult,
                std::move(DefaultValue)
            });
            AdvanceGraphVariableIdentifier();
            return Variable<T>(Identifier, m_Context);
        }

        template<typename T>
        [[nodiscard]] std::expected<void, DiagnosticCollection> BindInput(const NodeHandle& DestinationNode, PinIndex DestinationPin, const ValueOrExpr<T>& Expression)
        {
            if (m_IsClosed)
            {
                return std::unexpected(DiagnosticCollection{
                    MakeLifecycleDiagnostic("A closed graph builder cannot bind an input.")
                });
            }
            if (!IsHandleUsable(DestinationNode))
            {
                return std::unexpected(DiagnosticCollection{
                    MakeBindingDiagnostic("The destination node handle is invalid or belongs to another builder.")
                });
            }

            const NodeInstance* Destination = m_Graph.FindNode(DestinationNode.GetIdentifier());
            const NodeDescriptor* Descriptor = m_Descriptors.Find(Destination->Descriptor);
            if (Descriptor == nullptr)
            {
                return std::unexpected(DiagnosticCollection{
                    MakeMissingDescriptorDiagnostic(
                        "The input destination node descriptor is no longer registered.")
                });
            }
            if (!DestinationPin.IsValid() || DestinationPin.GetValue() >= Descriptor->GetPins().size())
            {
                return std::unexpected(DiagnosticCollection{
                    MakePinDiagnostic("The requested destination pin does not exist.")
                });
            }
            const PinSchema& Pin = Descriptor->GetPins()[DestinationPin.GetValue()];
            if (Pin.GetDirection() != PinDirection::Input || Pin.GetCategory() != PinCategory::Data)
            {
                return std::unexpected(DiagnosticCollection{
                    MakeBindingDiagnostic("An input binding destination must be a data input pin.")
                });
            }
            const auto TypeResult = GetCppTypeDesc<T>();
            if (!TypeResult.has_value())
            {
                return std::unexpected(TypeResult.error());
            }
            if (Pin.GetCardinality() != PinCardinality::Multiple &&
                CountBindings(DestinationNode.GetIdentifier(), DestinationPin) != 0U)
            {
                return std::unexpected(DiagnosticCollection{
                    Diagnostic{
                        .Severity = DiagnosticSeverity::Error,
                        .Code = DiagnosticCode::DuplicateInputBinding,
                        .Message = "A Single or Optional input pin already has a binding."
                    }
                });
            }

            InputBinding Binding;
            std::optional<TypeDesc> OutputTypeConstraint;
            const auto& Value = Expression.GetValue();
            if (const LiteralValue* Literal = std::get_if<LiteralValue>(&Value))
            {
                if (!Pin.AllowsLiteral())
                {
                    return std::unexpected(DiagnosticCollection{
                        MakeBindingDiagnostic("The destination pin does not allow literal bindings.")
                    });
                }
                if (!IsLiteralCompatible(*Literal, *TypeResult))
                {
                    return std::unexpected(DiagnosticCollection{
                        MakeTypeDiagnostic(
                            "The literal is incompatible with the ValueOrExpr C++ type.")
                    });
                }
                if (!IsLiteralCompatible(*Literal, Pin.GetType()))
                {
                    return std::unexpected(DiagnosticCollection{
                        MakeTypeDiagnostic("The literal is incompatible with the destination pin type.")
                    });
                }
                Binding = *Literal;
            }
            else if (const Output<T>* OutputValue = std::get_if<Output<T>>(&Value))
            {
                const auto OutputValidation = ValidateOutput(*OutputValue, Pin.GetType());
                if (!OutputValidation.empty())
                {
                    return std::unexpected(OutputValidation);
                }
                Binding = OutputReference{
                    OutputValue->GetIdentifier(),
                    OutputValue->GetPin()
                };
                OutputTypeConstraint = *TypeResult;
            }
            else
            {
                const Variable<T>& VariableValue = std::get<Variable<T>>(Value);
                const auto VariableValidation = ValidateVariable(VariableValue, Pin.GetType());
                if (!VariableValidation.empty())
                {
                    return std::unexpected(VariableValidation);
                }
                Binding = GraphVariableReference{VariableValue.GetIdentifier()};
            }

            m_Graph.BindInput(
                DestinationNode.GetIdentifier(),
                DestinationPin,
                std::move(Binding),
                std::move(OutputTypeConstraint)
            );
            return {};
        }

        [[nodiscard]] bool IsHandleUsable(const NodeHandle& Handle) const
        {
            if (m_IsClosed || !Handle.IsValid() || !HasSameContext(Handle))
            {
                return false;
            }

            const NodeInstance* Node = m_Graph.FindNode(Handle.GetIdentifier());
            return Node != nullptr && Node->Descriptor == Handle.GetDescriptor();
        }

        /// Checks open builder scopes, then delegates graph semantics to GraphIRValidator.
        [[nodiscard]] DiagnosticCollection Validate() const
        {
            if (m_IsClosed)
            {
                return DiagnosticCollection{
                    MakeLifecycleDiagnostic("A closed graph builder cannot be validated.")
                };
            }

            DiagnosticCollection Diagnostics = ValidateOpenExecutionState();
            DiagnosticCollection GraphDiagnostics = GraphIRValidator::Validate(m_Graph, m_Descriptors);
            Diagnostics.insert(Diagnostics.end(),
                std::make_move_iterator(GraphDiagnostics.begin()),
                std::make_move_iterator(GraphDiagnostics.end()));
            return Diagnostics;
        }

        /// Consumes the builder and invalidates its context before returning or
        /// reporting validation errors.
        [[nodiscard]] std::expected<GraphIR, DiagnosticCollection> Finalize() &&
        {
            if (m_IsClosed)
            {
                return std::unexpected(DiagnosticCollection{
                    MakeLifecycleDiagnostic("A closed graph builder cannot be finalized.")
                });
            }

            m_IsClosed = true;
            m_Context.reset();

            DiagnosticCollection Diagnostics = ValidateOpenExecutionState();
            DiagnosticCollection GraphDiagnostics = GraphIRValidator::Validate(m_Graph, m_Descriptors);
            Diagnostics.insert(Diagnostics.end(),
                std::make_move_iterator(GraphDiagnostics.begin()),
                std::make_move_iterator(GraphDiagnostics.end()));
            if (ContainsError(Diagnostics))
            {
                return std::unexpected(std::move(Diagnostics));
            }

            return std::move(m_Graph);
        }

    private:
#ifdef MILIASTRA_PHASE4_TEST_ACCESS
        friend struct GraphBuilderPhase4TestAccess;
#endif

        enum class ExecutionScopeKind
        {
            Entry,
            Branch,
            BranchArm,
            Loop
        };

        struct ExecutionScopeFrame
        {
            ExecutionScopeKind Kind;
            std::uint64_t Serial;
            std::uint64_t ParentSerial;
            ExecutionEntryId Entry;
            ExecutionRegionId Region;
            NodeInstanceId OwnerNode;
        };

        struct BranchConstructionState
        {
            NodeInstanceId Node;
            ExecutionEntryId Entry;
            ExecutionRegionId ParentRegion;
            ExecutionRegionId TrueRegion;
            ExecutionRegionId FalseRegion;
            std::uint64_t ParentSerial;
            std::uint64_t Serial;
            bool TrueOpened = false;
            bool FalseOpened = false;
            bool TrueResolved = false;
            bool FalseResolved = false;
            std::uint64_t TrueOutcomeSerial = 0U;
            std::uint64_t FalseOutcomeSerial = 0U;
        };

        struct OpenBranchOutcome
        {
            std::uint64_t Serial;
            std::uint64_t BranchSerial;
            std::uint64_t ParentScopeSerial;
            ExecutionEntryId Entry;
            ExecutionRegionId ParentRegion;
        };

        [[nodiscard]] bool IsBuilderOpen() const
        {
            return !m_IsClosed && m_Context != nullptr;
        }

        [[nodiscard]] bool HasSameContext(const std::weak_ptr<const NodeHandle::Context>& HandleContext) const
        {
            if (m_Context == nullptr || HandleContext.expired())
            {
                return false;
            }
            const std::weak_ptr<const NodeHandle::Context> BuilderContext = m_Context;
            return !BuilderContext.owner_before(HandleContext) &&
                !HandleContext.owner_before(BuilderContext);
        }

        [[nodiscard]] bool IsTopScope(ExecutionScopeKind Kind, std::uint64_t Serial) const
        {
            return !m_ExecutionScopes.empty() &&
                m_ExecutionScopes.back().Kind == Kind &&
                m_ExecutionScopes.back().Serial == Serial;
        }

        [[nodiscard]] ExecutionScopeKind CurrentParentScopeKind() const
        {
            if (m_ExecutionScopes.empty())
            {
                return ExecutionScopeKind::Entry;
            }
            return m_ExecutionScopes.back().Kind;
        }

        [[nodiscard]] bool IsValidActiveScope(
            ExecutionScopeKind Kind,
            std::uint64_t Serial,
            std::weak_ptr<const NodeHandle::Context> ScopeContext,
            ExecutionEntryId Entry,
            ExecutionRegionId Region
        ) const
        {
            if (!IsBuilderOpen() || Serial == 0U || !HasSameContext(ScopeContext) ||
                !IsTopScope(Kind, Serial))
            {
                return false;
            }
            const ExecutionScopeFrame& Frame = m_ExecutionScopes.back();
            return Frame.Entry == Entry && Frame.Region == Region;
        }

        [[nodiscard]] const NodeDescriptor* FindValidDescriptor(NodeDescriptorId Identifier) const
        {
            if (!Identifier.IsValid())
            {
                return nullptr;
            }
            const NodeDescriptor* Descriptor = m_Descriptors.Find(Identifier);
            return Descriptor != nullptr && Descriptor->IsValid() ? Descriptor : nullptr;
        }

        template<typename Schema>
        [[nodiscard]] static const Schema* GetControlSchema(const NodeDescriptor& Descriptor)
        {
            const std::optional<ExecutionControlSchema>& ControlSchema =
                Descriptor.GetExecutionControlSchema();
            return ControlSchema.has_value() ? std::get_if<Schema>(&*ControlSchema) : nullptr;
        }

        [[nodiscard]] static bool HasFlowPins(const NodeDescriptor& Descriptor)
        {
            for (const PinSchema& Pin : Descriptor.GetPins())
            {
                if (Pin.GetCategory() == PinCategory::Execution ||
                    Pin.GetType() == TypeDesc::Flow())
                {
                    return true;
                }
            }
            return false;
        }

        [[nodiscard]] static bool IsValidBranchArm(BranchArm Arm)
        {
            switch (Arm)
            {
            case BranchArm::True:
            case BranchArm::False:
                return true;
            }
            return false;
        }

        [[nodiscard]] BranchConstructionState* FindBranchState(std::uint64_t Serial)
        {
            for (BranchConstructionState& State : m_BranchStates)
            {
                if (State.Serial == Serial)
                {
                    return &State;
                }
            }
            return nullptr;
        }

        [[nodiscard]] const BranchConstructionState* FindBranchState(std::uint64_t Serial) const
        {
            for (const BranchConstructionState& State : m_BranchStates)
            {
                if (State.Serial == Serial)
                {
                    return &State;
                }
            }
            return nullptr;
        }

        [[nodiscard]] bool HasOpenBranchOutcome(ExecutionEntryId Entry) const
        {
            for (const OpenBranchOutcome& Outcome : m_OpenBranchOutcomes)
            {
                if (Outcome.Entry == Entry)
                {
                    return true;
                }
            }
            return false;
        }

        [[nodiscard]] const OpenBranchOutcome* FindOpenBranchOutcome(std::uint64_t Serial) const
        {
            for (const OpenBranchOutcome& Outcome : m_OpenBranchOutcomes)
            {
                if (Outcome.Serial == Serial)
                {
                    return &Outcome;
                }
            }
            return nullptr;
        }

        void ConsumeOpenBranchOutcome(std::uint64_t Serial)
        {
            for (std::size_t Index = 0U; Index < m_OpenBranchOutcomes.size(); ++Index)
            {
                if (m_OpenBranchOutcomes[Index].Serial == Serial)
                {
                    m_OpenBranchOutcomes.erase(m_OpenBranchOutcomes.begin() +
                        static_cast<std::ptrdiff_t>(Index));
                    return;
                }
            }
        }

        [[nodiscard]] std::size_t CountOutgoingEdges(NodeInstanceId Node, PinIndex OutputPin) const
        {
            std::size_t Count = 0U;
            for (const ControlEdge& Edge : m_Graph.GetControlEdges())
            {
                if (Edge.SourceNode == Node && Edge.SourceOutputPin == OutputPin)
                {
                    ++Count;
                }
            }
            return Count;
        }

        [[nodiscard]] bool IsRegionWithin(ExecutionRegionId RegionIdentifier, ExecutionRegionId AncestorIdentifier) const
        {
            const ExecutionRegion* Region = m_Graph.FindExecutionRegion(RegionIdentifier);
            std::vector<ExecutionRegionId> Visited;
            while (Region != nullptr)
            {
                if (Region->Identifier == AncestorIdentifier)
                {
                    return true;
                }
                if (std::find(Visited.begin(), Visited.end(), Region->Identifier) != Visited.end())
                {
                    return false;
                }
                Visited.push_back(Region->Identifier);
                if (!Region->Parent.has_value())
                {
                    return false;
                }
                Region = m_Graph.FindExecutionRegion(*Region->Parent);
            }
            return false;
        }

        [[nodiscard]] bool HasOpenBranchOutcomeInRegion(ExecutionRegionId RegionIdentifier) const
        {
            for (const OpenBranchOutcome& Outcome : m_OpenBranchOutcomes)
            {
                if (Outcome.ParentRegion.IsValid() &&
                    IsRegionWithin(Outcome.ParentRegion, RegionIdentifier))
                {
                    return true;
                }
            }
            return false;
        }

        [[nodiscard]] bool HasReachableLoopBreak(NodeInstanceId LoopNodeIdentifier, const LoopControlSchema& LoopSchema, ExecutionRegionId BodyRegionIdentifier) const
        {
            std::vector<NodeInstanceId> Reachable;
            Reachable.push_back(LoopNodeIdentifier);
            for (std::size_t Cursor = 0U; Cursor < Reachable.size(); ++Cursor)
            {
                const NodeInstanceId Current = Reachable[Cursor];
                for (const ControlEdge& Edge : m_Graph.GetControlEdges())
                {
                    if (Edge.SourceNode != Current)
                    {
                        continue;
                    }
                    const NodeInstance* Destination = m_Graph.FindNode(Edge.DestinationNode);
                    const NodeDescriptor* DestinationDescriptor = Destination == nullptr
                        ? nullptr : m_Descriptors.Find(Destination->Descriptor);
                    const LoopControlSchema* DestinationLoop = DestinationDescriptor == nullptr
                        ? nullptr : GetControlSchema<LoopControlSchema>(*DestinationDescriptor);
                    if (DestinationLoop != nullptr &&
                        (Edge.DestinationInputPin == DestinationLoop->RepeatInput ||
                            Edge.DestinationInputPin == DestinationLoop->BreakInput))
                    {
                        continue;
                    }
                    bool Seen = false;
                    for (const NodeInstanceId Existing : Reachable)
                    {
                        Seen = Seen || Existing == Edge.DestinationNode;
                    }
                    if (!Seen && Destination != nullptr)
                    {
                        Reachable.push_back(Destination->Identifier);
                    }
                }
            }

            for (const ControlEdge& Edge : m_Graph.GetControlEdges())
            {
                if (Edge.DestinationNode != LoopNodeIdentifier ||
                    Edge.DestinationInputPin != LoopSchema.BreakInput)
                {
                    continue;
                }
                const NodeInstance* Source = m_Graph.FindNode(Edge.SourceNode);
                const bool IsBodyOutputTransfer = Edge.SourceNode == LoopNodeIdentifier &&
                    Edge.SourceOutputPin == LoopSchema.BodyOutput &&
                    BodyRegionIdentifier.IsValid();
                if (Source == nullptr || (!IsBodyOutputTransfer &&
                    (!Source->ExecutionRegion.has_value() ||
                        !IsRegionWithin(*Source->ExecutionRegion, BodyRegionIdentifier))))
                {
                    continue;
                }
                for (const NodeInstanceId ReachableNode : Reachable)
                {
                    if (ReachableNode == Source->Identifier)
                    {
                        return true;
                    }
                }
            }
            return false;
        }

        [[nodiscard]] bool IsBranchArmOutput(NodeInstanceId Node, PinIndex OutputPin, ExecutionRegionId Region) const
        {
            const ExecutionRegion* ArmRegion = m_Graph.FindExecutionRegion(Region);
            return ArmRegion != nullptr && ArmRegion->Kind == ExecutionRegionKind::BranchArm &&
                ArmRegion->OwnerNode == Node && ArmRegion->OwnerOutputPin == OutputPin;
        }

        [[nodiscard]] DiagnosticCollection ValidateExecutionHandle(const ExecutionHandle& Handle, ExecutionEntryId Entry, ExecutionRegionId Region) const
        {
            if (!Handle.IsValid())
            {
                return DiagnosticCollection{
                    MakeExecutionHandleDiagnostic("The execution handle is invalid or stale.")
                };
            }
            if (!HasSameContext(Handle.m_Context))
            {
                return DiagnosticCollection{
                    MakeForeignContextDiagnostic("The execution handle belongs to another builder.")
                };
            }
            if (Handle.m_Entry != Entry || Handle.m_Region != Region)
            {
                return DiagnosticCollection{
                    MakeExecutionHandleDiagnostic("The execution handle belongs to a different entry or region.")
                };
            }
            const NodeInstance* Node = m_Graph.FindNode(Handle.m_Node);
            const NodeDescriptor* Descriptor = Node == nullptr
                ? nullptr : m_Descriptors.Find(Node->Descriptor);
            if (Node == nullptr || Descriptor == nullptr || !Descriptor->IsValid() ||
                Handle.m_OutputPin.GetValue() >= Descriptor->GetPins().size())
            {
                return DiagnosticCollection{
                    MakeExecutionHandleDiagnostic("The execution endpoint no longer resolves to a valid node pin.")
                };
            }
            const PinSchema& Pin = Descriptor->GetPins()[Handle.m_OutputPin.GetValue()];
            if (Pin.GetDirection() != PinDirection::Output ||
                Pin.GetCategory() != PinCategory::Execution || Pin.GetType() != TypeDesc::Flow())
            {
                return DiagnosticCollection{
                    MakeExecutionRoleDiagnostic("An execution handle must identify a descriptor-declared Flow output.")
                };
            }
            const ExecutionRegion* HandleRegion = m_Graph.FindExecutionRegion(Region);
            if (HandleRegion == nullptr || HandleRegion->Entry != Entry)
            {
                return DiagnosticCollection{
                    MakeExecutionHandleDiagnostic("The execution handle region is missing or belongs to another entry.")
                };
            }
            if (const BranchControlSchema* Branch = GetControlSchema<BranchControlSchema>(*Descriptor))
            {
                const bool IsDeclaredArmOutput = Handle.m_OutputPin == Branch->TrueOutput ||
                    Handle.m_OutputPin == Branch->FalseOutput;
                if (!IsDeclaredArmOutput || !IsBranchArmOutput(Handle.m_Node,
                    Handle.m_OutputPin, Region))
                {
                    return DiagnosticCollection{
                        MakeExecutionHandleDiagnostic("A Branch output handle must carry its exact owned arm region.")
                    };
                }
            }
            else if (const LoopControlSchema* Loop = GetControlSchema<LoopControlSchema>(*Descriptor);
                Loop != nullptr && Handle.m_OutputPin == Loop->BodyOutput)
            {
                const ExecutionRegion* BodyRegion = m_Graph.FindExecutionRegion(Region);
                if (BodyRegion == nullptr || BodyRegion->Kind != ExecutionRegionKind::LoopBody ||
                    BodyRegion->OwnerNode != Handle.m_Node ||
                    BodyRegion->OwnerOutputPin != Loop->BodyOutput)
                {
                    return DiagnosticCollection{
                        MakeExecutionHandleDiagnostic("A Loop Body output handle must carry its exact owned LoopBody region.")
                    };
                }
            }
            else if (!Node->ExecutionRegion.has_value() || *Node->ExecutionRegion != Region)
            {
                return DiagnosticCollection{
                    MakeExecutionHandleDiagnostic("The execution handle region does not match its source node ownership.")
                };
            }
            if (CountOutgoingEdges(Handle.m_Node, Handle.m_OutputPin) != 0U)
            {
                return DiagnosticCollection{
                    MakeEndpointConsumedDiagnostic("The execution output endpoint already has a successor.")
                };
            }
            return {};
        }

        [[nodiscard]] DiagnosticCollection ValidateParentScope(
            ExecutionScopeKind Kind,
            std::uint64_t Serial,
            std::weak_ptr<const NodeHandle::Context> ScopeContext,
            ExecutionEntryId Entry,
            ExecutionRegionId Region,
            std::uint64_t AllowedOutcomeSerial = 0U
        ) const
        {
            if (!IsBuilderOpen())
            {
                return DiagnosticCollection{
                    MakeLifecycleDiagnostic("A closed graph builder cannot construct execution nodes.")
                };
            }
            if (Serial == 0U || ScopeContext.expired())
            {
                return DiagnosticCollection{
                    MakeExecutionScopeDiagnostic("The execution scope is invalid or stale.")
                };
            }
            if (!HasSameContext(ScopeContext))
            {
                return DiagnosticCollection{
                    MakeForeignContextDiagnostic("The execution scope belongs to another builder.")
                };
            }
            if (!IsTopScope(Kind, Serial) || m_ExecutionScopes.back().Entry != Entry ||
                m_ExecutionScopes.back().Region != Region)
            {
                return DiagnosticCollection{
                    MakeExecutionScopeDiagnostic("The execution scope is not the active innermost region.")
                };
            }
            for (const OpenBranchOutcome& Outcome : m_OpenBranchOutcomes)
            {
                if (Outcome.Entry == Entry && Outcome.Serial != AllowedOutcomeSerial)
                {
                    return DiagnosticCollection{
                        MakeBranchOutcomeDiagnostic("Resolve the current branch outcome before appending another node.")
                    };
                }
            }
            return {};
        }

        [[nodiscard]] DiagnosticCollection ValidateLoopScopeForTransfer(const LoopScope& Scope, const ExecutionHandle& Tail) const
        {
            if (!IsBuilderOpen() || !Scope.IsValid())
            {
                return DiagnosticCollection{
                    MakeExecutionScopeDiagnostic("The target LoopScope is invalid or stale.")
                };
            }
            if (!HasSameContext(Scope.m_Context))
            {
                return DiagnosticCollection{
                    MakeForeignContextDiagnostic("The target LoopScope belongs to another builder.")
                };
            }
            std::optional<std::size_t> TargetIndex;
            std::optional<std::size_t> NearestLoopIndex;
            for (std::size_t Index = 0U; Index < m_ExecutionScopes.size(); ++Index)
            {
                const ExecutionScopeFrame& Frame = m_ExecutionScopes[Index];
                if (Frame.Kind == ExecutionScopeKind::Loop)
                {
                    NearestLoopIndex = Index;
                    if (Frame.Serial == Scope.m_Serial && Frame.OwnerNode == Scope.m_Node &&
                        Frame.Entry == Scope.m_Entry && Frame.Region == Scope.m_BodyRegion &&
                        Frame.ParentSerial == Scope.m_ParentSerial)
                    {
                        TargetIndex = Index;
                    }
                }
            }
            if (!TargetIndex.has_value() || !NearestLoopIndex.has_value() ||
                *TargetIndex != *NearestLoopIndex)
            {
                return DiagnosticCollection{
                    MakeLoopTransferDiagnostic("Break and Continue must target the nearest active enclosing LoopScope.")
                };
            }
            if (m_ExecutionScopes.empty())
            {
                return DiagnosticCollection{
                    MakeExecutionScopeDiagnostic("A loop transfer requires an active body or arm scope.")
                };
            }
            const ExecutionScopeFrame& Current = m_ExecutionScopes.back();
            if ((Current.Kind != ExecutionScopeKind::Loop &&
                    Current.Kind != ExecutionScopeKind::BranchArm) ||
                Current.Entry != Scope.m_Entry ||
                !IsRegionWithin(Current.Region, Scope.m_BodyRegion))
            {
                return DiagnosticCollection{
                    MakeLoopTransferDiagnostic("The active construction path is outside the target LoopBody.")
                };
            }
            if (Tail.m_Entry != Current.Entry || Tail.m_Region != Current.Region)
            {
                return DiagnosticCollection{
                    MakeExecutionHandleDiagnostic("A loop transfer tail must belong to the active execution region.")
                };
            }
            return ValidateExecutionHandle(Tail, Scope.m_Entry, Current.Region);
        }

        [[nodiscard]] std::expected<void, DiagnosticCollection> AddLoopTransfer(LoopScope& Scope, const ExecutionHandle& Tail, bool IsBreak)
        {
            DiagnosticCollection ScopeDiagnostics = ValidateLoopScopeForTransfer(Scope, Tail);
            if (!ScopeDiagnostics.empty())
            {
                return std::unexpected(std::move(ScopeDiagnostics));
            }
            const NodeInstance* LoopNode = m_Graph.FindNode(Scope.m_Node);
            const NodeDescriptor* Descriptor = LoopNode == nullptr
                ? nullptr : m_Descriptors.Find(LoopNode->Descriptor);
            const LoopControlSchema* Schema = Descriptor == nullptr
                ? nullptr : GetControlSchema<LoopControlSchema>(*Descriptor);
            if (Schema == nullptr)
            {
                return std::unexpected(DiagnosticCollection{
                    MakeExecutionRoleDiagnostic("A loop transfer requires the target's trusted Loop control schema.")
                });
            }
            const PinIndex DestinationPin = IsBreak ? Schema->BreakInput : Schema->RepeatInput;
            m_Graph.AddControlEdge(ControlEdge{
                Tail.m_Node, Tail.m_OutputPin, Scope.m_Node, DestinationPin
            });
            return {};
        }

        [[nodiscard]] DiagnosticCollection DescriptorFailure(NodeDescriptorId Identifier, const char* Message) const
        {
            if (!Identifier.IsValid())
            {
                return DiagnosticCollection{MakeDescriptorDiagnostic(Message)};
            }
            if (m_Descriptors.Find(Identifier) == nullptr)
            {
                return DiagnosticCollection{MakeMissingDescriptorDiagnostic(Message)};
            }
            return DiagnosticCollection{MakeDescriptorDiagnostic(Message)};
        }

        [[nodiscard]] static Diagnostic MakeExecutionHandleDiagnostic(const char* Message)
        {
            return MakeDiagnostic(DiagnosticCode::InvalidExecutionHandle, Message);
        }

        [[nodiscard]] static Diagnostic MakeForeignContextDiagnostic(const char* Message)
        {
            return MakeDiagnostic(DiagnosticCode::ForeignBuilderContext, Message);
        }

        [[nodiscard]] static Diagnostic MakeEndpointConsumedDiagnostic(const char* Message)
        {
            return MakeDiagnostic(DiagnosticCode::ExecutionEndpointAlreadyConsumed, Message);
        }

        [[nodiscard]] static Diagnostic MakeExecutionScopeDiagnostic(const char* Message)
        {
            return MakeDiagnostic(DiagnosticCode::InvalidExecutionScope, Message);
        }

        [[nodiscard]] static Diagnostic MakeExecutionRoleDiagnostic(const char* Message)
        {
            return MakeDiagnostic(DiagnosticCode::InvalidExecutionControlRole, Message);
        }

        [[nodiscard]] static Diagnostic MakeBranchArmDiagnostic(const char* Message)
        {
            return MakeDiagnostic(DiagnosticCode::InvalidBranchArmState, Message);
        }

        [[nodiscard]] static Diagnostic MakeBranchOutcomeDiagnostic(const char* Message)
        {
            return MakeDiagnostic(DiagnosticCode::InvalidBranchOutcome, Message);
        }

        [[nodiscard]] static Diagnostic MakeMissingJoinDiagnostic(const char* Message)
        {
            return MakeDiagnostic(DiagnosticCode::MissingExplicitJoin, Message);
        }

        [[nodiscard]] static Diagnostic MakeOpenExecutionScopeDiagnostic(const char* Message)
        {
            return MakeDiagnostic(DiagnosticCode::OpenExecutionScope, Message);
        }

        [[nodiscard]] static Diagnostic MakeExecutionReachabilityDiagnostic(const char* Message)
        {
            return MakeDiagnostic(DiagnosticCode::InvalidExecutionReachability, Message);
        }

        [[nodiscard]] static Diagnostic MakeExecutionOwnershipDiagnostic(const char* Message)
        {
            return MakeDiagnostic(DiagnosticCode::InvalidExecutionOwnership, Message);
        }

        [[nodiscard]] static Diagnostic MakeDiagnostic(DiagnosticCode Code, const char* Message)
        {
            return Diagnostic{
                .Severity = DiagnosticSeverity::Error,
                .Code = Code,
                .Message = Message
            };
        }

        [[nodiscard]] static Diagnostic MakeDataDominanceDiagnostic(const char* Message)
        {
            return MakeDiagnostic(DiagnosticCode::ExecutionDataNotDominated, Message);
        }

        [[nodiscard]] static Diagnostic MakeLoopTransferDiagnostic(const char* Message)
        {
            return MakeDiagnostic(DiagnosticCode::InvalidLoopTransfer, Message);
        }

        [[nodiscard]] std::expected<ExecutionNodeResult, DiagnosticCollection> AppendSequence(
            ExecutionEntryId Entry,
            ExecutionRegionId Region,
            std::uint64_t ScopeSerial,
            std::weak_ptr<const NodeHandle::Context> ScopeContext,
            const ExecutionHandle& Predecessor,
            NodeDescriptorId SequenceDescriptorIdentifier
        )
        {
            const ExecutionScopeKind ScopeKind = CurrentParentScopeKind();
            DiagnosticCollection ScopeDiagnostics = ValidateParentScope(ScopeKind,
                ScopeSerial, std::move(ScopeContext), Entry, Region);
            if (!ScopeDiagnostics.empty())
            {
                return std::unexpected(std::move(ScopeDiagnostics));
            }
            DiagnosticCollection HandleDiagnostics = ValidateExecutionHandle(
                Predecessor, Entry, Region);
            if (!HandleDiagnostics.empty())
            {
                return std::unexpected(std::move(HandleDiagnostics));
            }
            const NodeDescriptor* Descriptor = FindValidDescriptor(SequenceDescriptorIdentifier);
            if (Descriptor == nullptr)
            {
                return std::unexpected(DescriptorFailure(SequenceDescriptorIdentifier,
                    "The requested Sequence descriptor is missing or invalid."));
            }
            const SequenceControlSchema* Schema = GetControlSchema<SequenceControlSchema>(*Descriptor);
            if (Schema == nullptr)
            {
                return std::unexpected(DiagnosticCollection{
                    MakeExecutionRoleDiagnostic("AppendExecutionNode requires a trusted Sequence control schema.")
                });
            }
            if (m_NextNodeIdentifier == 0U)
            {
                return std::unexpected(DiagnosticCollection{
                    MakeLifecycleDiagnostic("The graph builder exhausted its node identifier range.")
                });
            }

            const NodeInstanceId NodeIdentifier(m_NextNodeIdentifier);
            m_Graph.AddNode(NodeInstance{NodeIdentifier, SequenceDescriptorIdentifier, Region});
            m_Graph.AddControlEdge(ControlEdge{
                Predecessor.m_Node,
                Predecessor.m_OutputPin,
                NodeIdentifier,
                Schema->ExecutionInput
            });
            AdvanceIdentifier(m_NextNodeIdentifier);

            NodeHandle Node(NodeIdentifier, SequenceDescriptorIdentifier, m_Context);
            ExecutionHandle Output(NodeIdentifier, Schema->ExecutionOutput, Entry,
                Region, m_Context);
            return ExecutionNodeResult{std::move(Node), std::move(Output)};
        }

        // Keep every check above the first graph mutation so a failed Return leaves Tail reusable.
        [[nodiscard]] std::expected<void, DiagnosticCollection> ReturnInScope(
            ExecutionScopeKind ScopeKind,
            ExecutionEntryId Entry,
            ExecutionRegionId Region,
            std::uint64_t ScopeSerial,
            std::weak_ptr<const NodeHandle::Context> ScopeContext,
            const ExecutionHandle& Tail,
            NodeDescriptorId ReturnDescriptorIdentifier
        )
        {
            DiagnosticCollection ScopeDiagnostics = ValidateParentScope(
                ScopeKind, ScopeSerial, std::move(ScopeContext), Entry, Region);
            if (!ScopeDiagnostics.empty())
            {
                return std::unexpected(std::move(ScopeDiagnostics));
            }

            DiagnosticCollection HandleDiagnostics = ValidateExecutionHandle(Tail, Entry, Region);
            if (!HandleDiagnostics.empty())
            {
                return std::unexpected(std::move(HandleDiagnostics));
            }

            const NodeDescriptor* Descriptor = FindValidDescriptor(ReturnDescriptorIdentifier);
            if (Descriptor == nullptr)
            {
                return std::unexpected(DescriptorFailure(ReturnDescriptorIdentifier,
                    "The requested Return descriptor is missing or invalid."));
            }
            const ReturnControlSchema* Schema =
                GetControlSchema<ReturnControlSchema>(*Descriptor);
            if (Schema == nullptr)
            {
                return std::unexpected(DiagnosticCollection{
                    MakeExecutionRoleDiagnostic(
                        "Return requires a trusted Return control schema.")
                });
            }
            if (m_NextNodeIdentifier == 0U)
            {
                return std::unexpected(DiagnosticCollection{
                    MakeLifecycleDiagnostic("The graph builder exhausted its node identifier range.")
                });
            }

            const NodeInstanceId NodeIdentifier(m_NextNodeIdentifier);
            m_Graph.AddNode(NodeInstance{
                NodeIdentifier,
                ReturnDescriptorIdentifier,
                Region
            });
            m_Graph.AddControlEdge(ControlEdge{
                Tail.m_Node,
                Tail.m_OutputPin,
                NodeIdentifier,
                Schema->ExecutionInput
            });
            AdvanceIdentifier(m_NextNodeIdentifier);
            return {};
        }

        template<typename T>
        [[nodiscard]] std::expected<InputBindingRecord, DiagnosticCollection> PrepareInputBindingRecord(
            NodeInstanceId DestinationNode,
            PinIndex DestinationPin,
            const PinSchema& Pin,
            const ValueOrExpr<T>& Expression) const
        {
            if (Pin.GetDirection() != PinDirection::Input || Pin.GetCategory() != PinCategory::Data)
            {
                return std::unexpected(DiagnosticCollection{
                    MakeBindingDiagnostic("An input binding destination must be a data input pin.")
                });
            }
            const auto TypeResult = GetCppTypeDesc<T>();
            if (!TypeResult.has_value())
            {
                return std::unexpected(TypeResult.error());
            }
            if (Pin.GetCardinality() != PinCardinality::Multiple &&
                CountBindings(DestinationNode, DestinationPin) != 0U)
            {
                return std::unexpected(DiagnosticCollection{
                    MakeDiagnostic(DiagnosticCode::DuplicateInputBinding,
                        "A Single or Optional input pin already has a binding.")
                });
            }

            InputBinding Binding;
            std::optional<TypeDesc> OutputTypeConstraint;
            const auto& Value = Expression.GetValue();
            if (const LiteralValue* Literal = std::get_if<LiteralValue>(&Value))
            {
                if (!Pin.AllowsLiteral())
                {
                    return std::unexpected(DiagnosticCollection{
                        MakeBindingDiagnostic("The destination pin does not allow literal bindings.")
                    });
                }
                if (!IsLiteralCompatible(*Literal, *TypeResult) ||
                    !IsLiteralCompatible(*Literal, Pin.GetType()))
                {
                    return std::unexpected(DiagnosticCollection{
                        MakeTypeDiagnostic("The literal is incompatible with its ValueOrExpr or destination type.")
                    });
                }
                Binding = *Literal;
            }
            else if (const Output<T>* OutputValue = std::get_if<Output<T>>(&Value))
            {
                const DiagnosticCollection Validation = ValidateOutput(*OutputValue, Pin.GetType());
                if (!Validation.empty())
                {
                    return std::unexpected(Validation);
                }
                Binding = OutputReference{OutputValue->GetIdentifier(), OutputValue->GetPin()};
                OutputTypeConstraint = *TypeResult;
            }
            else
            {
                const Variable<T>& VariableValue = std::get<Variable<T>>(Value);
                const DiagnosticCollection Validation = ValidateVariable(VariableValue, Pin.GetType());
                if (!Validation.empty())
                {
                    return std::unexpected(Validation);
                }
                Binding = GraphVariableReference{VariableValue.GetIdentifier()};
            }

            return InputBindingRecord{
                DestinationNode,
                DestinationPin,
                std::move(Binding),
                std::move(OutputTypeConstraint)
            };
        }

        [[nodiscard]] bool ExecutionProducerDominates(ExecutionEntryId Entry, NodeInstanceId Producer, NodeInstanceId Consumer) const
        {
            const ExecutionEntry* EntryRecord = m_Graph.FindExecutionEntry(Entry);
            if (EntryRecord == nullptr)
            {
                return false;
            }
            std::vector<NodeInstanceId> AllNodes;
            for (const NodeInstance& Node : m_Graph.GetNodes())
            {
                if (!Node.ExecutionRegion.has_value())
                {
                    continue;
                }
                const ExecutionRegion* Region = m_Graph.FindExecutionRegion(*Node.ExecutionRegion);
                const NodeDescriptor* Descriptor = m_Descriptors.Find(Node.Descriptor);
                if (Region != nullptr && Region->Entry == Entry && Descriptor != nullptr &&
                    Descriptor->GetExecutionControlSchema().has_value())
                {
                    AllNodes.push_back(Node.Identifier);
                }
            }
            std::vector<std::pair<std::size_t, std::size_t>> Edges;
            const auto FindIndex = [&AllNodes](NodeInstanceId Node)
            {
                for (std::size_t Index = 0U; Index < AllNodes.size(); ++Index)
                {
                    if (AllNodes[Index] == Node)
                    {
                        return Index;
                    }
                }
                return AllNodes.size();
            };
            for (const ControlEdge& Edge : m_Graph.GetControlEdges())
            {
                const std::size_t SourceIndex = FindIndex(Edge.SourceNode);
                if (SourceIndex == AllNodes.size())
                {
                    continue;
                }
                const NodeInstance* Destination = m_Graph.FindNode(Edge.DestinationNode);
                const NodeDescriptor* DestinationDescriptor = Destination == nullptr
                    ? nullptr : m_Descriptors.Find(Destination->Descriptor);
                const LoopControlSchema* DestinationLoop = DestinationDescriptor == nullptr
                    ? nullptr : GetControlSchema<LoopControlSchema>(*DestinationDescriptor);
                if (DestinationLoop != nullptr && Edge.DestinationInputPin == DestinationLoop->RepeatInput)
                {
                    continue;
                }
                if (DestinationLoop != nullptr && Edge.DestinationInputPin == DestinationLoop->BreakInput)
                {
                    for (const ControlEdge& ExitEdge : m_Graph.GetControlEdges())
                    {
                        if (ExitEdge.SourceNode == Destination->Identifier &&
                            ExitEdge.SourceOutputPin == DestinationLoop->ExitOutput)
                        {
                            const std::size_t ExitIndex = FindIndex(ExitEdge.DestinationNode);
                            if (ExitIndex != AllNodes.size())
                            {
                                Edges.emplace_back(SourceIndex, ExitIndex);
                            }
                        }
                    }
                    continue;
                }
                const std::size_t DestinationIndex = FindIndex(Edge.DestinationNode);
                if (DestinationIndex == AllNodes.size())
                {
                    continue;
                }
                const LoopControlSchema* SourceLoop = FindLoopSchemaInBuilder(Edge.SourceNode);
                if (SourceLoop != nullptr && Edge.SourceOutputPin == SourceLoop->ExitOutput &&
                    SourceLoop->ExitPolicy == LoopExitPolicy::Unconditional &&
                    !HasAnyLoopBreakEdge(Edge.SourceNode, *SourceLoop))
                {
                    continue;
                }
                Edges.emplace_back(SourceIndex, DestinationIndex);
            }

            const std::size_t RootIndex = FindIndex(EntryRecord->RootNode);
            const std::size_t ProducerIndex = FindIndex(Producer);
            const std::size_t ConsumerIndex = FindIndex(Consumer);
            if (RootIndex == AllNodes.size() || ProducerIndex == AllNodes.size() ||
                ConsumerIndex == AllNodes.size())
            {
                return false;
            }
            std::vector<std::size_t> Reachable{RootIndex};
            for (std::size_t Cursor = 0U; Cursor < Reachable.size(); ++Cursor)
            {
                for (const auto& [Source, Destination] : Edges)
                {
                    if (Source == Reachable[Cursor] &&
                        std::find(Reachable.begin(), Reachable.end(), Destination) == Reachable.end())
                    {
                        Reachable.push_back(Destination);
                    }
                }
            }
            if (std::find(Reachable.begin(), Reachable.end(), ProducerIndex) == Reachable.end() ||
                std::find(Reachable.begin(), Reachable.end(), ConsumerIndex) == Reachable.end())
            {
                return false;
            }
            std::vector<std::vector<bool>> Dominators(AllNodes.size(), std::vector<bool>(AllNodes.size(), true));
            for (std::size_t Candidate = 0U; Candidate < AllNodes.size(); ++Candidate)
            {
                Dominators[RootIndex][Candidate] = Candidate == RootIndex;
            }
            bool Changed = true;
            while (Changed)
            {
                Changed = false;
                for (const std::size_t NodeIndex : Reachable)
                {
                    if (NodeIndex == RootIndex)
                    {
                        continue;
                    }
                    std::vector<std::size_t> Predecessors;
                    for (const auto& [Source, Destination] : Edges)
                    {
                        if (Destination == NodeIndex &&
                            std::find(Reachable.begin(), Reachable.end(), Source) != Reachable.end())
                        {
                            Predecessors.push_back(Source);
                        }
                    }
                    std::vector<bool> Next(AllNodes.size(), false);
                    Next[NodeIndex] = true;
                    if (!Predecessors.empty())
                    {
                        for (std::size_t Candidate = 0U; Candidate < AllNodes.size(); ++Candidate)
                        {
                            bool InAll = true;
                            for (const std::size_t Predecessor : Predecessors)
                            {
                                InAll = InAll && Dominators[Predecessor][Candidate];
                            }
                            Next[Candidate] = Next[Candidate] || InAll;
                        }
                    }
                    if (Next != Dominators[NodeIndex])
                    {
                        Dominators[NodeIndex] = std::move(Next);
                        Changed = true;
                    }
                }
            }
            return Dominators[ConsumerIndex][ProducerIndex];
        }

        [[nodiscard]] const LoopControlSchema* FindLoopSchemaInBuilder(NodeInstanceId NodeIdentifier) const
        {
            const NodeInstance* Node = m_Graph.FindNode(NodeIdentifier);
            const NodeDescriptor* Descriptor = Node == nullptr
                ? nullptr : m_Descriptors.Find(Node->Descriptor);
            return Descriptor == nullptr
                ? nullptr : GetControlSchema<LoopControlSchema>(*Descriptor);
        }

        [[nodiscard]] bool HasAnyLoopBreakEdge(NodeInstanceId LoopNode, const LoopControlSchema& Schema) const
        {
            for (const ControlEdge& Edge : m_Graph.GetControlEdges())
            {
                if (Edge.DestinationNode == LoopNode &&
                    Edge.DestinationInputPin == Schema.BreakInput)
                {
                    return true;
                }
            }
            return false;
        }

        [[nodiscard]] DiagnosticCollection ValidateLoopConditionProvenance(const ValueOrExpr<bool>& Condition, ExecutionEntryId Entry, NodeInstanceId Predecessor) const
        {
            const Output<bool>* OutputValue = std::get_if<Output<bool>>(&Condition.GetValue());
            if (OutputValue == nullptr)
            {
                return {};
            }
            std::vector<NodeInstanceId> Pending{OutputValue->GetIdentifier()};
            std::vector<NodeInstanceId> Visited;
            while (!Pending.empty())
            {
                const NodeInstanceId Current = Pending.back();
                Pending.pop_back();
                if (std::find(Visited.begin(), Visited.end(), Current) != Visited.end())
                {
                    continue;
                }
                Visited.push_back(Current);
                const NodeInstance* Node = m_Graph.FindNode(Current);
                const NodeDescriptor* Descriptor = Node == nullptr
                    ? nullptr : m_Descriptors.Find(Node->Descriptor);
                if (Node == nullptr || Descriptor == nullptr || !Descriptor->IsValid())
                {
                    return DiagnosticCollection{
                        MakeExecutionHandleDiagnostic("A Loop condition depends on an unresolved data producer.")
                    };
                }
                if (Descriptor->GetExecutionControlSchema().has_value())
                {
                    const ExecutionRegion* ProducerRegion = Node->ExecutionRegion.has_value()
                        ? m_Graph.FindExecutionRegion(*Node->ExecutionRegion) : nullptr;
                    if (ProducerRegion == nullptr || ProducerRegion->Entry != Entry ||
                        !ExecutionProducerDominates(Entry, Current, Predecessor))
                    {
                        return DiagnosticCollection{
                            MakeDataDominanceDiagnostic("A Loop condition producer must belong to the same entry and dominate the pre-test.")
                        };
                    }
                    continue;
                }
                for (const InputBindingRecord& Binding : m_Graph.GetInputBindings())
                {
                    if (Binding.DestinationNode != Current)
                    {
                        continue;
                    }
                    if (const OutputReference* Reference =
                        std::get_if<OutputReference>(&Binding.Binding))
                    {
                        Pending.push_back(Reference->SourceNode);
                    }
                }
            }
            return {};
        }

        [[nodiscard]] std::expected<LoopStart, DiagnosticCollection> BeginLoopInScope(
            ExecutionEntryId Entry,
            ExecutionRegionId ParentRegion,
            std::uint64_t ParentSerial,
            std::weak_ptr<const NodeHandle::Context> ParentContext,
            const ExecutionHandle& Predecessor,
            NodeDescriptorId LoopDescriptorIdentifier,
            std::optional<ValueOrExpr<bool>> Condition
        )
        {
            DiagnosticCollection ScopeDiagnostics = ValidateParentScope(
                CurrentParentScopeKind(), ParentSerial, std::move(ParentContext), Entry,
                ParentRegion);
            if (!ScopeDiagnostics.empty())
            {
                return std::unexpected(std::move(ScopeDiagnostics));
            }
            DiagnosticCollection HandleDiagnostics = ValidateExecutionHandle(
                Predecessor, Entry, ParentRegion);
            if (!HandleDiagnostics.empty())
            {
                return std::unexpected(std::move(HandleDiagnostics));
            }
            const NodeDescriptor* Descriptor = FindValidDescriptor(LoopDescriptorIdentifier);
            if (Descriptor == nullptr)
            {
                return std::unexpected(DescriptorFailure(LoopDescriptorIdentifier,
                    "The requested Loop descriptor is missing or invalid."));
            }
            const LoopControlSchema* Schema = GetControlSchema<LoopControlSchema>(*Descriptor);
            if (Schema == nullptr)
            {
                return std::unexpected(DiagnosticCollection{
                    MakeExecutionRoleDiagnostic("BeginLoop requires a trusted Loop control schema.")
                });
            }
            if (m_NextNodeIdentifier == 0U || m_NextExecutionRegionIdentifier == 0U ||
                m_NextExecutionScopeSerial == 0U)
            {
                return std::unexpected(DiagnosticCollection{
                    MakeLifecycleDiagnostic("The graph builder exhausted a Loop construction identifier range.")
                });
            }

            const NodeInstanceId NodeIdentifier(m_NextNodeIdentifier);
            const ExecutionRegionId BodyRegionIdentifier(m_NextExecutionRegionIdentifier);
            const std::uint64_t ScopeSerial = m_NextExecutionScopeSerial;
            std::optional<InputBindingRecord> PreparedCondition;
            if (Schema->ExitPolicy == LoopExitPolicy::Conditional)
            {
                if (!Schema->ConditionInput.has_value())
                {
                    return std::unexpected(DiagnosticCollection{
                        MakeExecutionRoleDiagnostic("A Conditional Loop schema must declare its Boolean condition input.")
                    });
                }
                const PinSchema& ConditionPin = Descriptor->GetPins()[Schema->ConditionInput->GetValue()];
                if (Condition.has_value())
                {
                    const DiagnosticCollection Provenance = ValidateLoopConditionProvenance(
                        *Condition, Entry, Predecessor.m_Node);
                    if (!Provenance.empty())
                    {
                        return std::unexpected(Provenance);
                    }
                    auto Binding = PrepareInputBindingRecord(NodeIdentifier,
                        *Schema->ConditionInput, ConditionPin, *Condition);
                    if (!Binding.has_value())
                    {
                        return std::unexpected(std::move(Binding.error()));
                    }
                    PreparedCondition = std::move(*Binding);
                }
                else if (!ConditionPin.GetDefaultValue().has_value())
                {
                    return std::unexpected(DiagnosticCollection{
                        MakeBindingDiagnostic("A Conditional Loop requires a Boolean condition or compatible descriptor default.")
                    });
                }
            }
            else if (Condition.has_value())
            {
                return std::unexpected(DiagnosticCollection{
                    MakeBindingDiagnostic("An Unconditional Loop cannot bind a condition.")
                });
            }

            m_Graph.AddNode(NodeInstance{NodeIdentifier, LoopDescriptorIdentifier, ParentRegion});
            m_Graph.AddControlEdge(ControlEdge{
                Predecessor.m_Node, Predecessor.m_OutputPin, NodeIdentifier,
                Schema->ExecutionInput
            });
            if (PreparedCondition.has_value())
            {
                m_Graph.BindInput(PreparedCondition->DestinationNode,
                    PreparedCondition->DestinationInputPin, PreparedCondition->Binding,
                    PreparedCondition->OutputTypeConstraint);
            }
            m_Graph.AddExecutionRegion(ExecutionRegion{
                BodyRegionIdentifier,
                Entry,
                ExecutionRegionKind::LoopBody,
                ParentRegion,
                NodeIdentifier,
                Schema->BodyOutput
            });
            m_ExecutionScopes.push_back(ExecutionScopeFrame{
                ExecutionScopeKind::Loop,
                ScopeSerial,
                ParentSerial,
                Entry,
                BodyRegionIdentifier,
                NodeIdentifier
            });
            AdvanceIdentifier(m_NextNodeIdentifier);
            AdvanceIdentifier(m_NextExecutionRegionIdentifier);
            AdvanceIdentifier(m_NextExecutionScopeSerial);

            LoopScope Scope(NodeIdentifier, Entry, ParentRegion, BodyRegionIdentifier,
                ParentSerial, ScopeSerial, m_Context);
            NodeHandle Node(NodeIdentifier, LoopDescriptorIdentifier, m_Context);
            ExecutionHandle BodyOutput(NodeIdentifier, Schema->BodyOutput, Entry,
                BodyRegionIdentifier, m_Context);
            return LoopStart{std::move(Scope), std::move(Node), std::move(BodyOutput)};
        }

        [[nodiscard]] std::expected<BranchStart, DiagnosticCollection> BeginBranchInScope(
            ExecutionEntryId Entry,
            ExecutionRegionId ParentRegion,
            std::uint64_t ParentSerial,
            std::weak_ptr<const NodeHandle::Context> ParentContext,
            const ExecutionHandle& Predecessor,
            NodeDescriptorId BranchDescriptorIdentifier,
            const ValueOrExpr<bool>& Condition
        )
        {
            const ExecutionScopeKind ParentKind = CurrentParentScopeKind();
            DiagnosticCollection ScopeDiagnostics = ValidateParentScope(ParentKind,
                ParentSerial, std::move(ParentContext), Entry, ParentRegion);
            if (!ScopeDiagnostics.empty())
            {
                return std::unexpected(std::move(ScopeDiagnostics));
            }
            DiagnosticCollection HandleDiagnostics = ValidateExecutionHandle(
                Predecessor, Entry, ParentRegion);
            if (!HandleDiagnostics.empty())
            {
                return std::unexpected(std::move(HandleDiagnostics));
            }
            const NodeDescriptor* Descriptor = FindValidDescriptor(BranchDescriptorIdentifier);
            if (Descriptor == nullptr)
            {
                return std::unexpected(DescriptorFailure(BranchDescriptorIdentifier,
                    "The requested Branch descriptor is missing or invalid."));
            }
            const BranchControlSchema* Schema = GetControlSchema<BranchControlSchema>(*Descriptor);
            if (Schema == nullptr)
            {
                return std::unexpected(DiagnosticCollection{
                    MakeExecutionRoleDiagnostic("BeginBranch requires a trusted Branch control schema.")
                });
            }
            if (m_NextNodeIdentifier == 0U || m_NextExecutionRegionIdentifier == 0U ||
                m_NextExecutionScopeSerial == 0U)
            {
                return std::unexpected(DiagnosticCollection{
                    MakeLifecycleDiagnostic("The graph builder exhausted an identifier range.")
                });
            }

            const NodeInstanceId NodeIdentifier(m_NextNodeIdentifier);
            const ExecutionRegionId TrueRegion(m_NextExecutionRegionIdentifier);
            const std::uint64_t RegionAfterTrue = NextIdentifier(m_NextExecutionRegionIdentifier);
            if (RegionAfterTrue == 0U)
            {
                return std::unexpected(DiagnosticCollection{
                    MakeLifecycleDiagnostic("The graph builder cannot allocate a second branch region.")
                });
            }
            const ExecutionRegionId FalseRegion(RegionAfterTrue);
            const std::uint64_t BranchSerial = m_NextExecutionScopeSerial;
            const std::uint64_t ScopeAfterBranch = NextIdentifier(BranchSerial);
            if (ScopeAfterBranch == 0U)
            {
                return std::unexpected(DiagnosticCollection{
                    MakeLifecycleDiagnostic("The graph builder exhausted its execution scope serial range.")
                });
            }
            if (m_NextBranchOutcomeSerial == 0U)
            {
                return std::unexpected(DiagnosticCollection{
                    MakeLifecycleDiagnostic("The graph builder exhausted its branch outcome serial range.")
                });
            }
            const auto PreparedBinding = PrepareInputBindingRecord(
                NodeIdentifier, Schema->ConditionInput,
                Descriptor->GetPins()[Schema->ConditionInput.GetValue()], Condition);
            if (!PreparedBinding.has_value())
            {
                return std::unexpected(PreparedBinding.error());
            }

            m_Graph.AddNode(NodeInstance{NodeIdentifier, BranchDescriptorIdentifier, ParentRegion});
            m_Graph.BindInput(PreparedBinding->DestinationNode,
                PreparedBinding->DestinationInputPin, PreparedBinding->Binding,
                PreparedBinding->OutputTypeConstraint);
            m_Graph.AddControlEdge(ControlEdge{
                Predecessor.m_Node,
                Predecessor.m_OutputPin,
                NodeIdentifier,
                Schema->ExecutionInput
            });
            m_Graph.AddExecutionRegion(ExecutionRegion{
                TrueRegion, Entry, ExecutionRegionKind::BranchArm, ParentRegion,
                NodeIdentifier, Schema->TrueOutput
            });
            m_Graph.AddExecutionRegion(ExecutionRegion{
                FalseRegion, Entry, ExecutionRegionKind::BranchArm, ParentRegion,
                NodeIdentifier, Schema->FalseOutput
            });
            m_BranchStates.push_back(BranchConstructionState{
                NodeIdentifier, Entry, ParentRegion, TrueRegion, FalseRegion,
                ParentSerial, BranchSerial
            });
            m_ExecutionScopes.push_back(ExecutionScopeFrame{
                ExecutionScopeKind::Branch, BranchSerial, ParentSerial, Entry,
                ParentRegion, NodeIdentifier
            });
            AdvanceIdentifier(m_NextNodeIdentifier);
            AdvanceIdentifier(m_NextExecutionRegionIdentifier);
            AdvanceIdentifier(m_NextExecutionRegionIdentifier);
            AdvanceIdentifier(m_NextExecutionScopeSerial);

            BranchScope Scope(NodeIdentifier, Entry, ParentRegion, TrueRegion, FalseRegion,
                ParentSerial, BranchSerial, m_Context);
            NodeHandle Node(NodeIdentifier, BranchDescriptorIdentifier, m_Context);
            return BranchStart{std::move(Scope), std::move(Node)};
        }

        [[nodiscard]] std::expected<BranchArmOutcome, DiagnosticCollection> EndBranchArm(BranchArmScope&& Arm, const ExecutionHandle* LiveTail)
        {
            if (!IsBuilderOpen())
            {
                return std::unexpected(DiagnosticCollection{
                    MakeLifecycleDiagnostic("A closed graph builder cannot end a branch arm.")
                });
            }
            if (!Arm.IsValid())
            {
                return std::unexpected(DiagnosticCollection{
                    MakeExecutionScopeDiagnostic("The BranchArmScope is invalid or already closed.")
                });
            }
            if (!HasSameContext(Arm.m_Context))
            {
                return std::unexpected(DiagnosticCollection{
                    MakeForeignContextDiagnostic("The BranchArmScope belongs to another builder.")
                });
            }
            if (!IsTopScope(ExecutionScopeKind::BranchArm, Arm.m_Serial))
            {
                return std::unexpected(DiagnosticCollection{
                    MakeExecutionScopeDiagnostic("The BranchArmScope is not the active innermost scope.")
                });
            }
            BranchConstructionState* State = FindBranchState(Arm.m_BranchSerial);
            if (State == nullptr || State->Node != Arm.m_Branch || State->Entry != Arm.m_Entry)
            {
                return std::unexpected(DiagnosticCollection{
                    MakeBranchArmDiagnostic("The arm no longer belongs to an active branch.")
                });
            }
            const bool AlreadyResolved = Arm.m_Arm == BranchArm::True
                ? State->TrueResolved : State->FalseResolved;
            if (AlreadyResolved)
            {
                return std::unexpected(DiagnosticCollection{
                    MakeBranchArmDiagnostic("The semantic branch arm has already been resolved.")
                });
            }
            std::optional<ExecutionHandle> Tail;
            if (LiveTail != nullptr)
            {
                DiagnosticCollection HandleDiagnostics = ValidateExecutionHandle(
                    *LiveTail, Arm.m_Entry, Arm.m_Region);
                if (!HandleDiagnostics.empty())
                {
                    return std::unexpected(std::move(HandleDiagnostics));
                }
                Tail = *LiveTail;
            }
            if (m_NextBranchOutcomeSerial == 0U)
            {
                return std::unexpected(DiagnosticCollection{
                    MakeLifecycleDiagnostic("The graph builder exhausted its branch outcome serial range.")
                });
            }

            const std::uint64_t OutcomeSerial = m_NextBranchOutcomeSerial;
            if (Arm.m_Arm == BranchArm::True)
            {
                State->TrueResolved = true;
                State->TrueOutcomeSerial = OutcomeSerial;
            }
            else
            {
                State->FalseResolved = true;
                State->FalseOutcomeSerial = OutcomeSerial;
            }
            m_ExecutionScopes.pop_back();
            AdvanceIdentifier(m_NextBranchOutcomeSerial);
            BranchArmOutcome Outcome(Arm.m_Branch, Arm.m_Entry, Arm.m_ParentRegion,
                Arm.m_Region, Arm.m_Arm, std::move(Tail), Arm.m_BranchSerial,
                OutcomeSerial, m_Context);
            Arm.m_Serial = 0U;
            Arm.m_Context.reset();
            return Outcome;
        }

        [[nodiscard]] std::expected<BranchOutcome, DiagnosticCollection> EndBranchConstruction(BranchScope&& Branch, BranchArmOutcome&& TrueArm, BranchArmOutcome&& FalseArm)
        {
            if (!IsBuilderOpen())
            {
                return std::unexpected(DiagnosticCollection{
                    MakeLifecycleDiagnostic("A closed graph builder cannot end a branch.")
                });
            }
            if (!Branch.IsValid() || !TrueArm.IsValid() || !FalseArm.IsValid())
            {
                return std::unexpected(DiagnosticCollection{
                    MakeBranchOutcomeDiagnostic("EndBranch requires an active branch and two resolved arm outcomes.")
                });
            }
            if (!HasSameContext(Branch.m_Context) || !HasSameContext(TrueArm.m_Context) ||
                !HasSameContext(FalseArm.m_Context))
            {
                return std::unexpected(DiagnosticCollection{
                    MakeForeignContextDiagnostic("The branch or arm outcome belongs to another builder.")
                });
            }
            if (!IsTopScope(ExecutionScopeKind::Branch, Branch.m_Serial))
            {
                return std::unexpected(DiagnosticCollection{
                    MakeExecutionScopeDiagnostic("The BranchScope is not the active innermost scope.")
                });
            }
            const BranchConstructionState* State = FindBranchState(Branch.m_Serial);
            if (State == nullptr || !State->TrueResolved || !State->FalseResolved ||
                State->TrueOutcomeSerial != TrueArm.m_Serial ||
                State->FalseOutcomeSerial != FalseArm.m_Serial ||
                TrueArm.m_Arm != BranchArm::True || FalseArm.m_Arm != BranchArm::False ||
                TrueArm.m_BranchSerial != Branch.m_Serial ||
                FalseArm.m_BranchSerial != Branch.m_Serial ||
                TrueArm.m_Branch != Branch.m_Node || FalseArm.m_Branch != Branch.m_Node)
            {
                return std::unexpected(DiagnosticCollection{
                    MakeBranchOutcomeDiagnostic("EndBranch requires the semantic True and False outcomes of this branch.")
                });
            }
            if (m_NextBranchOutcomeSerial == 0U)
            {
                return std::unexpected(DiagnosticCollection{
                    MakeLifecycleDiagnostic("The graph builder exhausted its branch outcome serial range.")
                });
            }

            const std::uint64_t OutcomeSerial = m_NextBranchOutcomeSerial;
            const std::optional<ExecutionHandle> TrueTail = TrueArm.m_LiveTail;
            const std::optional<ExecutionHandle> FalseTail = FalseArm.m_LiveTail;
            const BranchConstructionState SavedState = *State;
            for (std::size_t Index = 0U; Index < m_BranchStates.size(); ++Index)
            {
                if (m_BranchStates[Index].Serial == Branch.m_Serial)
                {
                    m_BranchStates.erase(m_BranchStates.begin() +
                        static_cast<std::ptrdiff_t>(Index));
                    break;
                }
            }
            m_ExecutionScopes.pop_back();
            AdvanceIdentifier(m_NextBranchOutcomeSerial);

            BranchOutcome Outcome(Branch.m_Node, Branch.m_Entry, Branch.m_ParentRegion,
                Branch.m_TrueRegion, Branch.m_FalseRegion, TrueTail, FalseTail,
                Branch.m_Serial, OutcomeSerial, m_Context);
            if (Outcome.GetLiveArmCount() != 0U)
            {
                m_OpenBranchOutcomes.push_back(OpenBranchOutcome{
                    OutcomeSerial, SavedState.Serial, SavedState.ParentSerial,
                    SavedState.Entry, SavedState.ParentRegion
                });
            }
            Branch.m_Serial = 0U;
            Branch.m_Context.reset();
            TrueArm.m_Serial = 0U;
            TrueArm.m_Context.reset();
            FalseArm.m_Serial = 0U;
            FalseArm.m_Context.reset();
            return Outcome;
        }

        [[nodiscard]] std::expected<JoinResult, DiagnosticCollection> JoinBranchOutcome(
            ExecutionEntryId Entry,
            ExecutionRegionId ParentRegion,
            std::uint64_t ParentSerial,
            std::weak_ptr<const NodeHandle::Context> ParentContext,
            BranchOutcome&& Outcome,
            NodeDescriptorId JoinDescriptorIdentifier
        )
        {
            if (!Outcome.IsValid())
            {
                return std::unexpected(DiagnosticCollection{
                    MakeBranchOutcomeDiagnostic("Join requires a valid unconsumed BranchOutcome.")
                });
            }
            const OpenBranchOutcome* OpenOutcome = FindOpenBranchOutcome(Outcome.m_Serial);
            if (OpenOutcome == nullptr)
            {
                return std::unexpected(DiagnosticCollection{
                    MakeBranchOutcomeDiagnostic("The BranchOutcome is stale or has already been consumed.")
                });
            }
            DiagnosticCollection ScopeDiagnostics = ValidateParentScope(
                CurrentParentScopeKind(),
                ParentSerial, std::move(ParentContext), Entry, ParentRegion, Outcome.m_Serial);
            if (!ScopeDiagnostics.empty())
            {
                return std::unexpected(std::move(ScopeDiagnostics));
            }
            if (!HasSameContext(Outcome.m_Context))
            {
                return std::unexpected(DiagnosticCollection{
                    MakeForeignContextDiagnostic("The BranchOutcome belongs to another builder.")
                });
            }
            if (OpenOutcome->ParentScopeSerial != ParentSerial || Outcome.m_Entry != Entry ||
                Outcome.m_ParentRegion != ParentRegion || Outcome.GetLiveArmCount() != 2U ||
                !Outcome.m_TrueTail.has_value() || !Outcome.m_FalseTail.has_value())
            {
                return std::unexpected(DiagnosticCollection{
                    MakeBranchOutcomeDiagnostic("Join requires two live arms from a branch in this parent region.")
                });
            }
            DiagnosticCollection TrueValidation = ValidateExecutionHandle(
                *Outcome.m_TrueTail, Entry, Outcome.m_TrueRegion);
            if (!TrueValidation.empty())
            {
                return std::unexpected(std::move(TrueValidation));
            }
            DiagnosticCollection FalseValidation = ValidateExecutionHandle(
                *Outcome.m_FalseTail, Entry, Outcome.m_FalseRegion);
            if (!FalseValidation.empty())
            {
                return std::unexpected(std::move(FalseValidation));
            }
            const NodeDescriptor* Descriptor = FindValidDescriptor(JoinDescriptorIdentifier);
            if (Descriptor == nullptr)
            {
                return std::unexpected(DescriptorFailure(JoinDescriptorIdentifier,
                    "The requested Join descriptor is missing or invalid."));
            }
            const JoinControlSchema* Schema = GetControlSchema<JoinControlSchema>(*Descriptor);
            if (Schema == nullptr)
            {
                return std::unexpected(DiagnosticCollection{
                    MakeExecutionRoleDiagnostic("Join requires a trusted Join control schema.")
                });
            }
            if (m_NextNodeIdentifier == 0U)
            {
                return std::unexpected(DiagnosticCollection{
                    MakeLifecycleDiagnostic("The graph builder exhausted its node identifier range.")
                });
            }

            const NodeInstanceId NodeIdentifier(m_NextNodeIdentifier);
            m_Graph.AddNode(NodeInstance{NodeIdentifier, JoinDescriptorIdentifier, ParentRegion});
            m_Graph.AddControlEdge(ControlEdge{
                Outcome.m_TrueTail->m_Node, Outcome.m_TrueTail->m_OutputPin,
                NodeIdentifier, Schema->ExecutionInput
            });
            m_Graph.AddControlEdge(ControlEdge{
                Outcome.m_FalseTail->m_Node, Outcome.m_FalseTail->m_OutputPin,
                NodeIdentifier, Schema->ExecutionInput
            });
            AdvanceIdentifier(m_NextNodeIdentifier);
            ConsumeOpenBranchOutcome(Outcome.m_Serial);
            Outcome.m_Serial = 0U;
            Outcome.m_Context.reset();
            NodeHandle Node(NodeIdentifier, JoinDescriptorIdentifier, m_Context);
            ExecutionHandle Output(NodeIdentifier, Schema->ExecutionOutput, Entry,
                ParentRegion, m_Context);
            return JoinResult{std::move(Node), std::move(Output)};
        }

        [[nodiscard]] std::expected<ExecutionNodeResult, DiagnosticCollection> ContinueBranchOutcome(
            ExecutionEntryId Entry,
            ExecutionRegionId ParentRegion,
            std::uint64_t ParentSerial,
            std::weak_ptr<const NodeHandle::Context> ParentContext,
            BranchOutcome&& Outcome,
            NodeDescriptorId SequenceDescriptorIdentifier
        )
        {
            if (!Outcome.IsValid())
            {
                return std::unexpected(DiagnosticCollection{
                    MakeBranchOutcomeDiagnostic("ContinueWith requires a valid unconsumed BranchOutcome.")
                });
            }
            const OpenBranchOutcome* OpenOutcome = FindOpenBranchOutcome(Outcome.m_Serial);
            if (OpenOutcome == nullptr)
            {
                return std::unexpected(DiagnosticCollection{
                    MakeBranchOutcomeDiagnostic("The BranchOutcome is stale or has already been consumed.")
                });
            }
            DiagnosticCollection ScopeDiagnostics = ValidateParentScope(
                CurrentParentScopeKind(),
                ParentSerial, std::move(ParentContext), Entry, ParentRegion, Outcome.m_Serial);
            if (!ScopeDiagnostics.empty())
            {
                return std::unexpected(std::move(ScopeDiagnostics));
            }
            if (!HasSameContext(Outcome.m_Context))
            {
                return std::unexpected(DiagnosticCollection{
                    MakeForeignContextDiagnostic("The BranchOutcome belongs to another builder.")
                });
            }
            if (OpenOutcome->ParentScopeSerial != ParentSerial || Outcome.m_Entry != Entry ||
                Outcome.m_ParentRegion != ParentRegion || Outcome.GetLiveArmCount() != 1U)
            {
                return std::unexpected(DiagnosticCollection{
                    MakeBranchOutcomeDiagnostic("ContinueWith requires exactly one live arm in this parent region.")
                });
            }
            const ExecutionHandle& Tail = Outcome.m_TrueTail.has_value()
                ? *Outcome.m_TrueTail : *Outcome.m_FalseTail;
            const ExecutionRegionId TailRegion = Outcome.m_TrueTail.has_value()
                ? Outcome.m_TrueRegion : Outcome.m_FalseRegion;
            DiagnosticCollection HandleDiagnostics = ValidateExecutionHandle(Tail, Entry, TailRegion);
            if (!HandleDiagnostics.empty())
            {
                return std::unexpected(std::move(HandleDiagnostics));
            }
            const NodeDescriptor* Descriptor = FindValidDescriptor(SequenceDescriptorIdentifier);
            if (Descriptor == nullptr)
            {
                return std::unexpected(DescriptorFailure(SequenceDescriptorIdentifier,
                    "The requested continuation Sequence descriptor is missing or invalid."));
            }
            const SequenceControlSchema* Schema = GetControlSchema<SequenceControlSchema>(*Descriptor);
            if (Schema == nullptr)
            {
                return std::unexpected(DiagnosticCollection{
                    MakeExecutionRoleDiagnostic("ContinueWith requires a trusted Sequence control schema.")
                });
            }
            if (m_NextNodeIdentifier == 0U)
            {
                return std::unexpected(DiagnosticCollection{
                    MakeLifecycleDiagnostic("The graph builder exhausted its node identifier range.")
                });
            }

            const NodeInstanceId NodeIdentifier(m_NextNodeIdentifier);
            m_Graph.AddNode(NodeInstance{NodeIdentifier, SequenceDescriptorIdentifier, ParentRegion});
            m_Graph.AddControlEdge(ControlEdge{
                Tail.m_Node, Tail.m_OutputPin, NodeIdentifier, Schema->ExecutionInput
            });
            AdvanceIdentifier(m_NextNodeIdentifier);
            ConsumeOpenBranchOutcome(Outcome.m_Serial);
            Outcome.m_Serial = 0U;
            Outcome.m_Context.reset();
            NodeHandle Node(NodeIdentifier, SequenceDescriptorIdentifier, m_Context);
            ExecutionHandle Output(NodeIdentifier, Schema->ExecutionOutput, Entry,
                ParentRegion, m_Context);
            return ExecutionNodeResult{std::move(Node), std::move(Output)};
        }

        [[nodiscard]] DiagnosticCollection ValidateOpenExecutionState() const
        {
            if (!m_ExecutionScopes.empty() || !m_BranchStates.empty() ||
                !m_OpenBranchOutcomes.empty())
            {
                return DiagnosticCollection{
                    MakeOpenExecutionScopeDiagnostic("The graph builder has an open execution scope or unresolved branch outcome.")
                };
            }
            return {};
        }

        [[nodiscard]] static std::uint64_t NextIdentifier(std::uint64_t Current)
        {
            if (Current == 0U || Current == std::numeric_limits<std::uint64_t>::max())
            {
                return 0U;
            }
            return Current + 1U;
        }

        static void AdvanceIdentifier(std::uint64_t& Current)
        {
            Current = NextIdentifier(Current);
        }

        [[nodiscard]] std::size_t CountBindings(NodeInstanceId DestinationNode, PinIndex DestinationPin) const
        {
            std::size_t Count = 0U;
            for (const InputBindingRecord& Record : m_Graph.GetInputBindings())
            {
                if (Record.DestinationNode == DestinationNode &&
                    Record.DestinationInputPin == DestinationPin)
                {
                    ++Count;
                }
            }
            return Count;
        }

        template<typename T>
        [[nodiscard]] DiagnosticCollection ValidateOutput(const Output<T>& OutputValue, const TypeDesc& DestinationType) const
        {
            const auto TypeResult = GetCppTypeDesc<T>();
            if (!TypeResult.has_value())
            {
                return TypeResult.error();
            }
            if (!OutputValue.IsValid() || !HasSameContext(OutputValue))
            {
                return DiagnosticCollection{
                    MakeBindingDiagnostic("The output handle is invalid or belongs to another builder.")
                };
            }
            const NodeInstance* SourceNode = m_Graph.FindNode(OutputValue.GetIdentifier());
            if (SourceNode == nullptr)
            {
                return DiagnosticCollection{
                    MakePinDiagnostic("The output handle references a missing node.")
                };
            }
            const NodeDescriptor* Descriptor = m_Descriptors.Find(SourceNode->Descriptor);
            if (Descriptor == nullptr)
            {
                return DiagnosticCollection{
                    MakeMissingDescriptorDiagnostic("The output source node descriptor is no longer registered.")
                };
            }
            if (OutputValue.GetPin().GetValue() >= Descriptor->GetPins().size())
            {
                return DiagnosticCollection{
                    MakePinDiagnostic("The output handle references a missing pin.")
                };
            }
            const PinSchema& SourcePin = Descriptor->GetPins()[OutputValue.GetPin().GetValue()];
            if (SourcePin.GetDirection() != PinDirection::Output ||
                SourcePin.GetCategory() != PinCategory::Data)
            {
                return DiagnosticCollection{
                    MakeBindingDiagnostic("An output binding must reference a data output pin.")
                };
            }
            if (!SourcePin.GetType().IsCompatibleWith(*TypeResult) ||
                !SourcePin.GetType().IsCompatibleWith(DestinationType))
            {
                return DiagnosticCollection{
                    MakeTypeDiagnostic("The output type is incompatible with the destination pin type.")
                };
            }
            return {};
        }

        template<typename T>
        [[nodiscard]] DiagnosticCollection ValidateVariable(const Variable<T>& VariableValue, const TypeDesc& DestinationType) const
        {
            const auto TypeResult = GetCppTypeDesc<T>();
            if (!TypeResult.has_value())
            {
                return TypeResult.error();
            }
            if (!VariableValue.IsValid() || !HasSameContext(VariableValue))
            {
                return DiagnosticCollection{
                    MakeBindingDiagnostic("The graph variable handle is invalid or belongs to another builder.")
                };
            }
            const GraphVariable* SourceVariable = m_Graph.FindVariable(VariableValue.GetIdentifier());
            if (SourceVariable == nullptr)
            {
                return DiagnosticCollection{
                    MakeVariableDiagnostic("The graph variable handle references a missing variable.")
                };
            }
            if (!SourceVariable->Type.IsCompatibleWith(*TypeResult) ||
                !SourceVariable->Type.IsCompatibleWith(DestinationType))
            {
                return DiagnosticCollection{
                    MakeTypeDiagnostic("The graph variable type is incompatible with the destination pin type.")
                };
            }
            return {};
        }

        template<typename T>
        [[nodiscard]] bool HasSameContext(const Output<T>& Handle) const
        {
            if (m_Context == nullptr)
            {
                return false;
            }
            const std::weak_ptr<const NodeHandle::Context> BuilderContext = m_Context;
            const std::weak_ptr<const NodeHandle::Context> HandleContext = Handle.m_Context;
            return !BuilderContext.owner_before(HandleContext) &&
                !HandleContext.owner_before(BuilderContext);
        }

        template<typename T>
        [[nodiscard]] bool HasSameContext(const Variable<T>& Handle) const
        {
            if (m_Context == nullptr)
            {
                return false;
            }
            const std::weak_ptr<const NodeHandle::Context> BuilderContext = m_Context;
            const std::weak_ptr<const NodeHandle::Context> HandleContext = Handle.m_Context;
            return !BuilderContext.owner_before(HandleContext) &&
                !HandleContext.owner_before(BuilderContext);
        }

        [[nodiscard]] static bool IsLiteralCompatible(const LiteralValue& Literal, const TypeDesc& Type)
        {
            if (!Literal.IsValid() || !Type.IsValid())
            {
                return false;
            }
            switch (Type.GetKind())
            {
            case TypeDesc::Kind::Boolean: return Literal.Is<bool>();
            case TypeDesc::Kind::Integer: return Literal.Is<std::int64_t>();
            case TypeDesc::Kind::Float: return Literal.Is<double>();
            case TypeDesc::Kind::String: return Literal.Is<std::string>();
            case TypeDesc::Kind::GUID: return Literal.Is<GuidValue>();
            case TypeDesc::Kind::Vector3: return Literal.Is<Vector3Value>();
            case TypeDesc::Kind::PrefabId: return Literal.Is<PrefabIdValue>();
            case TypeDesc::Kind::ConfigId: return Literal.Is<ConfigIdValue>();
            case TypeDesc::Kind::Faction: return Literal.Is<FactionValue>();
            case TypeDesc::Kind::Enum:
                return Literal.Is<EnumLiteralValue>() &&
                    Literal.TryGet<EnumLiteralValue>()->GetEnumTypeIdentity() ==
                    Type.GetEnumTypeIdentity();
            default: return false;
            }
        }

        [[nodiscard]] static Diagnostic MakeBindingDiagnostic(const char* Message)
        {
            return Diagnostic{
                .Severity = DiagnosticSeverity::Error,
                .Code = DiagnosticCode::InvalidInputBinding,
                .Message = Message
            };
        }

        [[nodiscard]] static Diagnostic MakePinDiagnostic(const char* Message)
        {
            return Diagnostic{
                .Severity = DiagnosticSeverity::Error,
                .Code = DiagnosticCode::InvalidGraphIRPinReference,
                .Message = Message
            };
        }

        [[nodiscard]] static Diagnostic MakeTypeDiagnostic(const char* Message)
        {
            return Diagnostic{
                .Severity = DiagnosticSeverity::Error,
                .Code = DiagnosticCode::IncompatibleGraphIRTypes,
                .Message = Message
            };
        }

        [[nodiscard]] static Diagnostic MakeVariableDiagnostic(const char* Message)
        {
            return Diagnostic{
                .Severity = DiagnosticSeverity::Error,
                .Code = DiagnosticCode::InvalidGraphVariable,
                .Message = Message
            };
        }

        [[nodiscard]] static Diagnostic MakeVariableNameDiagnostic(const char* Message)
        {
            return Diagnostic{
                .Severity = DiagnosticSeverity::Error,
                .Code = DiagnosticCode::DuplicateGraphVariableName,
                .Message = Message
            };
        }

        void AdvanceGraphVariableIdentifier()
        {
            if (m_NextGraphVariableIdentifier == std::numeric_limits<std::uint64_t>::max())
            {
                m_NextGraphVariableIdentifier = 0U;
            }
            else
            {
                ++m_NextGraphVariableIdentifier;
            }
        }

        [[nodiscard]] bool HasSameContext(const NodeHandle& Handle) const
        {
            if (m_Context == nullptr)
            {
                return false;
            }

            const std::weak_ptr<const NodeHandle::Context> BuilderContext = m_Context;
            return !BuilderContext.owner_before(Handle.m_Context) &&
                !Handle.m_Context.owner_before(BuilderContext);
        }

        [[nodiscard]] static Diagnostic MakeDescriptorDiagnostic(const char* Message)
        {
            return Diagnostic{
                .Severity = DiagnosticSeverity::Error,
                .Code = DiagnosticCode::InvalidNodeDescriptor,
                .Message = Message
            };
        }

        [[nodiscard]] static Diagnostic MakeMissingDescriptorDiagnostic(const char* Message)
        {
            return Diagnostic{
                .Severity = DiagnosticSeverity::Error,
                .Code = DiagnosticCode::MissingDescriptor,
                .Message = Message
            };
        }

        [[nodiscard]] static Diagnostic MakeLifecycleDiagnostic(const char* Message)
        {
            return Diagnostic{
                .Severity = DiagnosticSeverity::Error,
                .Code = DiagnosticCode::InvalidGraphBuilderState,
                .Message = Message
            };
        }

        const NodeDescriptorRegistry& m_Descriptors;
        GraphIR m_Graph;
        std::uint64_t m_NextNodeIdentifier = 1U;
        std::uint64_t m_NextGraphVariableIdentifier = 1U;
        std::uint64_t m_NextExecutionEntryIdentifier = 1U;
        std::uint64_t m_NextExecutionRegionIdentifier = 1U;
        std::uint64_t m_NextExecutionScopeSerial = 1U;
        std::uint64_t m_NextBranchOutcomeSerial = 1U;
        std::vector<ExecutionScopeFrame> m_ExecutionScopes;
        std::vector<BranchConstructionState> m_BranchStates;
        std::vector<OpenBranchOutcome> m_OpenBranchOutcomes;
        std::shared_ptr<const NodeHandle::Context> m_Context;
        bool m_IsClosed = false;
    };
}
