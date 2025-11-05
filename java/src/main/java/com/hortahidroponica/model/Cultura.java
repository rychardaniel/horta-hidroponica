package com.hortahidroponica.model;

import com.fasterxml.jackson.annotation.JsonManagedReference; // Importe esta classe
import io.quarkus.hibernate.orm.panache.PanacheEntityBase;
import jakarta.persistence.*;

import java.util.List;

@Entity
@Table(name = "\"CULTURA\"")
public class Cultura extends PanacheEntityBase {

    @Id
    @GeneratedValue(strategy = GenerationType.IDENTITY)
    @Column(name = "\"ID\"")
    public Integer id;

    @Column(name = "\"NOME\"", length = 100, unique = true, nullable = false)
    public String nome;

    @Column(name = "\"T_CULTIVO_DIAS\"", nullable = false)
    public Integer tempoCultivoDias;

    @OneToMany(mappedBy = "cultura", cascade = CascadeType.ALL, fetch = FetchType.LAZY)
    @JsonManagedReference
    public List<Parametro> parametros;
}